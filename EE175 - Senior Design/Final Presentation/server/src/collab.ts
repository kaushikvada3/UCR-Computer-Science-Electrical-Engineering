import * as Y from 'yjs'
import * as syncProtocol from 'y-protocols/sync'
import * as awarenessProtocol from 'y-protocols/awareness'
import * as encoding from 'lib0/encoding'
import * as decoding from 'lib0/decoding'
import type { IncomingMessage } from 'node:http'
import type { WebSocket } from 'ws'

type PersistenceLayer = {
  provider: unknown
  bindState: (docName: string, doc: WSSharedDoc) => void
  writeState: (docName: string, doc: WSSharedDoc) => Promise<unknown>
} | null

type AwarenessChanges = {
  added: number[]
  updated: number[]
  removed: number[]
}

const messageSync = 0
const messageAwareness = 1
const wsReadyStateConnecting = 0
const wsReadyStateOpen = 1
const pingTimeout = 30_000

let persistence: PersistenceLayer = null

const docs = new Map<string, WSSharedDoc>()

const toUint8Array = (data: Buffer | ArrayBuffer | Buffer[]) => {
  if (Array.isArray(data)) {
    return new Uint8Array(Buffer.concat(data))
  }
  if (data instanceof ArrayBuffer) {
    return new Uint8Array(data)
  }
  return new Uint8Array(data)
}

const send = (doc: WSSharedDoc, conn: WebSocket, message: Uint8Array) => {
  if (conn.readyState !== wsReadyStateConnecting && conn.readyState !== wsReadyStateOpen) {
    closeConn(doc, conn)
    return
  }
  try {
    conn.send(message, (error) => {
      if (error) {
        closeConn(doc, conn)
      }
    })
  } catch {
    closeConn(doc, conn)
  }
}

const updateHandler = (update: Uint8Array, _origin: unknown, doc: WSSharedDoc) => {
  const encoder = encoding.createEncoder()
  encoding.writeVarUint(encoder, messageSync)
  syncProtocol.writeUpdate(encoder, update)
  const message = encoding.toUint8Array(encoder)
  doc.conns.forEach((_state, conn) => {
    send(doc, conn, message)
  })
}

const closeConn = (doc: WSSharedDoc, conn: WebSocket) => {
  if (doc.conns.has(conn)) {
    const controlledIds = doc.conns.get(conn) ?? new Set<number>()
    doc.conns.delete(conn)
    awarenessProtocol.removeAwarenessStates(doc.awareness, Array.from(controlledIds), null)
    if (doc.conns.size === 0) {
      const writeState = persistence?.writeState(doc.name, doc) ?? Promise.resolve()
      void writeState.finally(() => {
        docs.delete(doc.name)
        doc.destroy()
      })
    }
  }
  conn.close()
}

const messageListener = (conn: WebSocket, doc: WSSharedDoc, message: Uint8Array) => {
  try {
    const encoder = encoding.createEncoder()
    const decoder = decoding.createDecoder(message)
    const messageType = decoding.readVarUint(decoder)
    switch (messageType) {
      case messageSync: {
        encoding.writeVarUint(encoder, messageSync)
        syncProtocol.readSyncMessage(decoder, encoder, doc, conn)
        if (encoding.length(encoder) > 1) {
          send(doc, conn, encoding.toUint8Array(encoder))
        }
        break
      }
      case messageAwareness: {
        awarenessProtocol.applyAwarenessUpdate(doc.awareness, decoding.readVarUint8Array(decoder), conn)
        break
      }
      default:
        break
    }
  } catch (error) {
    console.error('collab message error', error)
  }
}

export class WSSharedDoc extends Y.Doc {
  name: string
  conns: Map<WebSocket, Set<number>>
  awareness: awarenessProtocol.Awareness

  constructor(name: string, gc = true) {
    super({ gc })
    this.name = name
    this.conns = new Map()
    this.awareness = new awarenessProtocol.Awareness(this)
    this.awareness.setLocalState(null)

    this.awareness.on('update', ({ added, updated, removed }: AwarenessChanges, conn: WebSocket | null) => {
      const changedClients = added.concat(updated, removed)
      if (conn !== null) {
        const controlledIds = this.conns.get(conn)
        if (controlledIds) {
          added.forEach((clientId) => controlledIds.add(clientId))
          removed.forEach((clientId) => controlledIds.delete(clientId))
        }
      }
      const encoder = encoding.createEncoder()
      encoding.writeVarUint(encoder, messageAwareness)
      encoding.writeVarUint8Array(encoder, awarenessProtocol.encodeAwarenessUpdate(this.awareness, changedClients))
      const message = encoding.toUint8Array(encoder)
      this.conns.forEach((_state, currentConn) => {
        send(this, currentConn, message)
      })
    })

    this.on('update', (update: Uint8Array, origin: unknown, doc: Y.Doc) => {
      updateHandler(update, origin, doc as WSSharedDoc)
    })
  }
}

export const setPersistence = (nextPersistence: PersistenceLayer) => {
  persistence = nextPersistence
}

export const getYDoc = (docName: string, gc = true) => {
  let doc = docs.get(docName)
  if (!doc) {
    doc = new WSSharedDoc(docName, gc)
    persistence?.bindState(docName, doc)
    docs.set(docName, doc)
  }
  return doc
}

export const setupWSConnection = (
  conn: WebSocket,
  req: IncomingMessage,
  options: { docName?: string; gc?: boolean } = {},
) => {
  const docName = options.docName ?? (req.url || '').slice(1).split('?')[0]
  const gc = options.gc ?? true
  conn.binaryType = 'arraybuffer'

  const doc = getYDoc(docName, gc)
  doc.conns.set(conn, new Set())

  conn.on('message', (message) => {
    messageListener(conn, doc, toUint8Array(message as Buffer | ArrayBuffer | Buffer[]))
  })

  let pongReceived = true
  const pingInterval = setInterval(() => {
    if (!pongReceived) {
      if (doc.conns.has(conn)) {
        closeConn(doc, conn)
      }
      clearInterval(pingInterval)
      return
    }
    if (!doc.conns.has(conn)) {
      clearInterval(pingInterval)
      return
    }
    pongReceived = false
    try {
      conn.ping()
    } catch {
      closeConn(doc, conn)
      clearInterval(pingInterval)
    }
  }, pingTimeout)

  conn.on('close', () => {
    closeConn(doc, conn)
    clearInterval(pingInterval)
  })

  conn.on('pong', () => {
    pongReceived = true
  })

  const syncEncoder = encoding.createEncoder()
  encoding.writeVarUint(syncEncoder, messageSync)
  syncProtocol.writeSyncStep1(syncEncoder, doc)
  send(doc, conn, encoding.toUint8Array(syncEncoder))

  const awarenessStates = doc.awareness.getStates()
  if (awarenessStates.size > 0) {
    const awarenessEncoder = encoding.createEncoder()
    encoding.writeVarUint(awarenessEncoder, messageAwareness)
    encoding.writeVarUint8Array(
      awarenessEncoder,
      awarenessProtocol.encodeAwarenessUpdate(doc.awareness, Array.from(awarenessStates.keys())),
    )
    send(doc, conn, encoding.toUint8Array(awarenessEncoder))
  }
}
