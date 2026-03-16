import { DurableObject } from 'cloudflare:workers'
import * as Y from 'yjs'
import * as syncProtocol from 'y-protocols/sync'
import * as awarenessProtocol from 'y-protocols/awareness'
import * as encoding from 'lib0/encoding'
import * as decoding from 'lib0/decoding'
import type { DeckDocument } from '../../shared/src/deck.js'
import { seedDeck } from '../../shared/src/deck.js'
import { initializeDeckDoc, readDeckDoc, replaceDeckDoc } from './deck-doc'

export type SnapshotRecord = {
  id: string
  label: string
  createdAt: string
  autosave: boolean
}

type AwarenessChanges = {
  added: number[]
  updated: number[]
  removed: number[]
}

type SocketAttachment = {
  clientIds: number[]
  awareness: number[]
}

type PublishSnapshotRequest = {
  label?: string
  autosave?: boolean
  snapshot?: DeckDocument
}

const messageSync = 0
const messageAwareness = 1
const wsReadyStateConnecting = 0
const wsReadyStateOpen = 1
const documentStateKey = 'doc-state'
const snapshotHistoryKey = 'snapshot-history'
const snapshotKeyPrefix = 'snapshot:'

const emptyAttachment = (): SocketAttachment => ({
  clientIds: [],
  awareness: [],
})

const toUint8Array = (data: ArrayBuffer | string | Uint8Array) => {
  if (typeof data === 'string') {
    return new TextEncoder().encode(data)
  }
  if (data instanceof Uint8Array) {
    return data
  }
  return new Uint8Array(data)
}

const encodeForStorage = (value: Uint8Array) => Array.from(value)

const decodeFromStorage = (value: unknown) => {
  if (value instanceof Uint8Array) {
    return value
  }
  if (Array.isArray(value)) {
    return new Uint8Array(value as number[])
  }
  return null
}

const asJson = (payload: unknown, status = 200) => new Response(JSON.stringify(payload), {
  status,
  headers: {
    'Content-Type': 'application/json',
    'Cache-Control': 'no-store',
  },
})

const readAttachment = (socket: WebSocket): SocketAttachment => {
  const raw = socket.deserializeAttachment()
  if (!raw || typeof raw !== 'object') {
    return emptyAttachment()
  }
  const candidate = raw as Partial<SocketAttachment>
  return {
    clientIds: Array.isArray(candidate.clientIds)
      ? candidate.clientIds.filter((value): value is number => typeof value === 'number')
      : [],
    awareness: Array.isArray(candidate.awareness)
      ? candidate.awareness.filter((value): value is number => typeof value === 'number')
      : [],
  }
}

export class DeckRoom extends DurableObject {
  private readonly doc: Y.Doc
  private readonly awareness: awarenessProtocol.Awareness
  private readonly conns = new Map<WebSocket, Set<number>>()
  private persistTimer: ReturnType<typeof setTimeout> | null = null
  private rehydrating = true

  constructor(ctx: DurableObjectState, env: Env) {
    super(ctx, env)
    this.doc = new Y.Doc({ gc: true })
    this.awareness = new awarenessProtocol.Awareness(this.doc)
    this.awareness.setLocalState(null)
    this.installListeners()
    ctx.blockConcurrencyWhile(async () => {
      await this.loadDocument()
      this.restoreConnections()
      this.rehydrating = false
      this.broadcastCurrentAwareness()
    })
  }

  async fetch(request: Request): Promise<Response> {
    const url = new URL(request.url)

    if (request.headers.get('Upgrade') === 'websocket') {
      return this.handleWebsocket()
    }

    if (request.method === 'GET' && url.pathname.endsWith('/internal/history')) {
      return asJson({ snapshots: await this.listSnapshots() })
    }

    if (request.method === 'POST' && url.pathname.endsWith('/internal/publish-snapshot')) {
      const payload = await request.json<PublishSnapshotRequest>()
      const label = typeof payload.label === 'string' && payload.label.trim().length > 0
        ? payload.label.trim()
        : 'Snapshot'
      const autosave = Boolean(payload.autosave)
      const snapshot = payload.snapshot ?? this.getCurrentSnapshot()
      const record: SnapshotRecord = {
        id: autosave ? 'autosave' : crypto.randomUUID(),
        label,
        createdAt: new Date().toISOString(),
        autosave,
      }
      await this.saveSnapshot(record, snapshot)
      return asJson({ snapshot: record }, 201)
    }

    if (request.method === 'POST' && url.pathname.includes('/internal/restore-snapshot/')) {
      const snapshotId = decodeURIComponent(url.pathname.split('/internal/restore-snapshot/')[1] ?? '')
      if (!snapshotId) {
        return asJson({ error: 'Snapshot not found' }, 404)
      }
      const snapshot = await this.loadSnapshot(snapshotId)
      if (!snapshot) {
        return asJson({ error: 'Snapshot not found' }, 404)
      }
      replaceDeckDoc(this.doc, snapshot)
      await this.persistNow()
      return asJson({ restored: true })
    }

    return asJson({ error: 'Not found' }, 404)
  }

  webSocketMessage(socket: WebSocket, message: ArrayBuffer | string) {
    try {
      const encoder = encoding.createEncoder()
      const decoder = decoding.createDecoder(toUint8Array(message))
      const messageType = decoding.readVarUint(decoder)

      switch (messageType) {
        case messageSync: {
          encoding.writeVarUint(encoder, messageSync)
          syncProtocol.readSyncMessage(decoder, encoder, this.doc, socket)
          if (encoding.length(encoder) > 1) {
            this.send(socket, encoding.toUint8Array(encoder))
          }
          break
        }
        case messageAwareness: {
          awarenessProtocol.applyAwarenessUpdate(
            this.awareness,
            decoding.readVarUint8Array(decoder),
            socket,
          )
          break
        }
        default:
          break
      }
    } catch (error) {
      console.error('websocket message failed', error)
      this.closeConnection(socket)
    }
  }

  webSocketClose(socket: WebSocket) {
    this.closeConnection(socket)
    try {
      socket.close()
    } catch {
      // Socket is already closed.
    }
  }

  webSocketError(socket: WebSocket, error: unknown) {
    console.error('websocket error', error)
    this.closeConnection(socket)
  }

  private installListeners() {
    this.doc.on('update', (update: Uint8Array) => {
      if (this.rehydrating) {
        return
      }
      const encoder = encoding.createEncoder()
      encoding.writeVarUint(encoder, messageSync)
      syncProtocol.writeUpdate(encoder, update)
      const message = encoding.toUint8Array(encoder)
      this.broadcast(message)
      this.schedulePersist()
    })

    this.awareness.on('update', ({ added, updated, removed }: AwarenessChanges, origin: unknown) => {
      const changedClients = added.concat(updated, removed)
      if (origin instanceof WebSocket) {
        const controlledIds = this.conns.get(origin) ?? new Set<number>()
        added.forEach((clientId) => controlledIds.add(clientId))
        removed.forEach((clientId) => controlledIds.delete(clientId))
        this.conns.set(origin, controlledIds)
        this.writeAttachment(origin, controlledIds)
      }
      if (this.rehydrating) {
        return
      }
      const encoder = encoding.createEncoder()
      encoding.writeVarUint(encoder, messageAwareness)
      encoding.writeVarUint8Array(
        encoder,
        awarenessProtocol.encodeAwarenessUpdate(this.awareness, changedClients),
      )
      this.broadcast(encoding.toUint8Array(encoder))
    })
  }

  private async loadDocument() {
    const storedUpdate = await this.ctx.storage.get<number[]>(documentStateKey)
    const update = decodeFromStorage(storedUpdate)
    if (update) {
      Y.applyUpdate(this.doc, update)
    }
    if (this.doc.getArray('slides').length === 0) {
      initializeDeckDoc(this.doc, seedDeck)
      await this.persistNow()
    }
  }

  private restoreConnections() {
    const sockets = this.ctx.getWebSockets()
    sockets.forEach((socket) => {
      const attachment = readAttachment(socket)
      this.conns.set(socket, new Set(attachment.clientIds))
      if (attachment.awareness.length > 0) {
        awarenessProtocol.applyAwarenessUpdate(
          this.awareness,
          new Uint8Array(attachment.awareness),
          socket,
        )
      }
    })
  }

  private broadcastCurrentAwareness() {
    const states = this.awareness.getStates()
    if (states.size === 0) {
      return
    }
    const encoder = encoding.createEncoder()
    encoding.writeVarUint(encoder, messageAwareness)
    encoding.writeVarUint8Array(
      encoder,
      awarenessProtocol.encodeAwarenessUpdate(this.awareness, Array.from(states.keys())),
    )
    this.broadcast(encoding.toUint8Array(encoder))
  }

  private handleWebsocket() {
    const pair = new WebSocketPair()
    const client = pair[0]
    const server = pair[1]
    this.ctx.acceptWebSocket(server)
    this.conns.set(server, new Set())
    this.writeAttachment(server, new Set())
    this.sendInitialState(server)
    return new Response(null, {
      status: 101,
      webSocket: client,
    })
  }

  private sendInitialState(socket: WebSocket) {
    const syncEncoder = encoding.createEncoder()
    encoding.writeVarUint(syncEncoder, messageSync)
    syncProtocol.writeSyncStep1(syncEncoder, this.doc)
    this.send(socket, encoding.toUint8Array(syncEncoder))

    const awarenessStates = this.awareness.getStates()
    if (awarenessStates.size === 0) {
      return
    }
    const awarenessEncoder = encoding.createEncoder()
    encoding.writeVarUint(awarenessEncoder, messageAwareness)
    encoding.writeVarUint8Array(
      awarenessEncoder,
      awarenessProtocol.encodeAwarenessUpdate(this.awareness, Array.from(awarenessStates.keys())),
    )
    this.send(socket, encoding.toUint8Array(awarenessEncoder))
  }

  private send(socket: WebSocket, message: Uint8Array) {
    if (socket.readyState !== wsReadyStateConnecting && socket.readyState !== wsReadyStateOpen) {
      this.closeConnection(socket)
      return
    }
    try {
      socket.send(message)
    } catch {
      this.closeConnection(socket)
    }
  }

  private broadcast(message: Uint8Array) {
    this.conns.forEach((_clientIds, socket) => {
      this.send(socket, message)
    })
  }

  private closeConnection(socket: WebSocket) {
    if (!this.conns.has(socket)) {
      return
    }
    const controlledIds = Array.from(this.conns.get(socket) ?? [])
    this.conns.delete(socket)
    awarenessProtocol.removeAwarenessStates(this.awareness, controlledIds, null)
    if (this.conns.size === 0) {
      void this.persistNow()
    }
  }

  private writeAttachment(socket: WebSocket, clientIds: Set<number>) {
    const ids = Array.from(clientIds)
    const awareness = ids.length > 0
      ? encodeForStorage(awarenessProtocol.encodeAwarenessUpdate(this.awareness, ids))
      : []
    socket.serializeAttachment({
      clientIds: ids,
      awareness,
    } satisfies SocketAttachment)
  }

  private schedulePersist() {
    if (this.persistTimer) {
      clearTimeout(this.persistTimer)
    }
    this.persistTimer = setTimeout(() => {
      void this.persistNow()
    }, 350)
  }

  private async persistNow() {
    if (this.persistTimer) {
      clearTimeout(this.persistTimer)
      this.persistTimer = null
    }
    await this.ctx.storage.put(documentStateKey, encodeForStorage(Y.encodeStateAsUpdate(this.doc)))
  }

  private getCurrentSnapshot() {
    return readDeckDoc(this.doc)
  }

  private async listSnapshots() {
    return (await this.ctx.storage.get<SnapshotRecord[]>(snapshotHistoryKey)) ?? []
  }

  private async saveSnapshot(record: SnapshotRecord, snapshot: DeckDocument) {
    await this.ctx.storage.put(`${snapshotKeyPrefix}${record.id}`, snapshot)
    const nextHistory = [
      record,
      ...(await this.listSnapshots()).filter((entry) => entry.id !== record.id),
    ]
      .sort((left, right) => right.createdAt.localeCompare(left.createdAt))
      .slice(0, 40)
    await this.ctx.storage.put(snapshotHistoryKey, nextHistory)
  }

  private async loadSnapshot(snapshotId: string) {
    return (await this.ctx.storage.get<DeckDocument>(`${snapshotKeyPrefix}${snapshotId}`)) ?? null
  }
}
