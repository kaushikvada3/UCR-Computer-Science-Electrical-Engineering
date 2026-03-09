import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'
import type { DeckDocument } from '../../shared/src/deck.js'
import {
  DECK_ID,
  SHARE_TOKEN,
  seedDeck,
} from '../../shared/src/deck.js'
import {
  ServerYjs as Y,
  initializeServerDeckDoc,
  readServerDeckDoc,
  replaceServerDeckDoc,
} from './server-doc.js'

export type SnapshotRecord = {
  id: string
  label: string
  createdAt: string
  autosave: boolean
}

const __filename = fileURLToPath(import.meta.url)
const __dirname = path.dirname(__filename)
const projectRoot = path.resolve(__dirname, '..', '..')
const storageRoot = path.resolve(process.env.PERSIST_ROOT ?? path.join(projectRoot, 'server'))
const dataDir = path.resolve(process.env.DATA_DIR ?? path.join(storageRoot, 'data'))
const docsDir = path.resolve(process.env.DOCS_DIR ?? path.join(dataDir, 'docs'))
const snapshotsDir = path.resolve(process.env.SNAPSHOTS_DIR ?? path.join(dataDir, 'snapshots'))
const uploadsDir = path.resolve(process.env.UPLOADS_DIR ?? path.join(storageRoot, 'uploads'))

const persistTimers = new Map<string, NodeJS.Timeout>()

const ensureDir = (dir: string) => {
  if (!fs.existsSync(dir)) {
    fs.mkdirSync(dir, { recursive: true })
  }
}

export const ensureStorage = () => {
  ensureDir(dataDir)
  ensureDir(docsDir)
  ensureDir(snapshotsDir)
  ensureDir(uploadsDir)
  ensureDir(path.join(snapshotsDir, DECK_ID))
}

export const getProjectRoot = () => projectRoot
export const getUploadsDir = () => uploadsDir

const docStateFile = (docName: string) => path.join(docsDir, `${docName}.bin`)
const historyFile = (deckId: string) => path.join(snapshotsDir, deckId, 'history.json')
const snapshotFile = (deckId: string, snapshotId: string) => path.join(snapshotsDir, deckId, `${snapshotId}.json`)

const persistDocImmediately = (docName: string, doc: any) => {
  ensureStorage()
  const encoded = Buffer.from(Y.encodeStateAsUpdate(doc))
  fs.writeFileSync(docStateFile(docName), encoded)
}

export const schedulePersist = (docName: string, doc: any) => {
  const existing = persistTimers.get(docName)
  if (existing) {
    clearTimeout(existing)
  }
  const timer = setTimeout(() => {
    persistDocImmediately(docName, doc)
    persistTimers.delete(docName)
  }, 350)
  persistTimers.set(docName, timer)
}

export const createPersistence = () => ({
  provider: null,
  bindState: (docName: string, doc: any) => {
    ensureStorage()
    const target = docStateFile(docName)
    if (fs.existsSync(target)) {
      const update = fs.readFileSync(target)
      Y.applyUpdate(doc, new Uint8Array(update))
    }
    if (doc.getArray('slides').length === 0) {
      initializeServerDeckDoc(doc)
      persistDocImmediately(docName, doc)
    }
    doc.on('update', () => {
      schedulePersist(docName, doc)
    })
  },
  writeState: async (docName: string, doc: any) => {
    persistDocImmediately(docName, doc)
  },
})

const readHistory = (deckId: string): SnapshotRecord[] => {
  const target = historyFile(deckId)
  if (!fs.existsSync(target)) {
    return []
  }
  return JSON.parse(fs.readFileSync(target, 'utf8')) as SnapshotRecord[]
}

const writeHistory = (deckId: string, records: SnapshotRecord[]) => {
  ensureDir(path.join(snapshotsDir, deckId))
  fs.writeFileSync(historyFile(deckId), JSON.stringify(records, null, 2))
}

export const listSnapshots = (deckId: string) => readHistory(deckId)

export const saveSnapshot = (deckId: string, record: SnapshotRecord, snapshot: DeckDocument) => {
  ensureDir(path.join(snapshotsDir, deckId))
  fs.writeFileSync(snapshotFile(deckId, record.id), JSON.stringify(snapshot, null, 2))

  const existing = readHistory(deckId).filter((entry) => entry.id !== record.id)
  const next = [record, ...existing]
    .sort((left, right) => right.createdAt.localeCompare(left.createdAt))
    .slice(0, 40)
  writeHistory(deckId, next)
}

export const loadSnapshot = (deckId: string, snapshotId: string) => {
  const target = snapshotFile(deckId, snapshotId)
  if (!fs.existsSync(target)) {
    return null
  }
  return JSON.parse(fs.readFileSync(target, 'utf8')) as DeckDocument
}

export const bootstrapResponse = () => ({
  deckId: DECK_ID,
  roomId: DECK_ID,
  shareToken: SHARE_TOKEN,
  theme: seedDeck.theme,
  assetBaseUrl: '/assets',
  websocketPath: '/collab',
  collaboratorLabel: `Editor ${Math.floor(Math.random() * 900 + 100)}`,
})

export const replaceWithSnapshot = (doc: any, snapshot: DeckDocument) => {
  replaceServerDeckDoc(doc, snapshot)
  persistDocImmediately(DECK_ID, doc)
}

export const getCurrentSnapshot = (doc: any) => JSON.parse(JSON.stringify(readServerDeckDoc(doc))) as DeckDocument

export const getCurrentDeck = (doc: any) => readServerDeckDoc(doc)
