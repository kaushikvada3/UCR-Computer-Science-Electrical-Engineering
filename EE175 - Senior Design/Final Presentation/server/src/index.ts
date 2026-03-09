import path from 'node:path'
import http from 'node:http'
import { fileURLToPath } from 'node:url'
import type { IncomingMessage } from 'node:http'
import cors from 'cors'
import express from 'express'
import multer from 'multer'
import { nanoid } from 'nanoid'
import { WebSocketServer, type WebSocket } from 'ws'
import {
  DECK_ID,
  SHARE_TOKEN,
  type DeckDocument,
} from '../../shared/src/deck.js'
import { getYDoc, setPersistence, setupWSConnection } from './collab.js'
import {
  bootstrapResponse,
  createPersistence,
  ensureStorage,
  getCurrentSnapshot,
  getProjectRoot,
  getUploadsDir,
  listSnapshots,
  loadSnapshot,
  replaceWithSnapshot,
  saveSnapshot,
} from './storage.js'

const __filename = fileURLToPath(import.meta.url)
const __dirname = path.dirname(__filename)

ensureStorage()
setPersistence(createPersistence() as any)

const app = express()
const server = http.createServer(app)
const wss = new WebSocketServer({ noServer: true })
const projectRoot = getProjectRoot()
const uploadsDir = getUploadsDir()
const appDistDir = path.join(projectRoot, 'app', 'dist')
const port = Number(process.env.PORT ?? 8787)

const storage = multer.diskStorage({
  destination: (_req, _file, callback) => {
    callback(null, uploadsDir)
  },
  filename: (_req, file, callback) => {
    const extension = path.extname(file.originalname)
    callback(null, `${nanoid(10)}${extension}`)
  },
})

const upload = multer({ storage })

app.use(cors())
app.use(express.json({ limit: '8mb' }))
app.use('/assets/uploads', express.static(uploadsDir))
app.use('/assets', express.static(projectRoot))

app.get('/api/health', (_req, res) => {
  res.json({ ok: true })
})

app.get('/api/share/:token/bootstrap', (req, res) => {
  if (req.params.token !== SHARE_TOKEN) {
    res.status(404).json({ error: 'Unknown share link' })
    return
  }
  res.json(bootstrapResponse())
})

app.post('/api/decks/:deckId/assets', upload.single('file'), (req, res) => {
  if (req.params.deckId !== DECK_ID) {
    res.status(404).json({ error: 'Unknown deck' })
    return
  }
  if (!req.file) {
    res.status(400).json({ error: 'Missing file upload' })
    return
  }
  const isModel = path.extname(req.file.originalname).toLowerCase() === '.glb'
  res.json({
    assetId: nanoid(10),
    filename: req.file.filename,
    originalName: req.file.originalname,
    kind: isModel ? 'model' : 'image',
    url: `/assets/uploads/${req.file.filename}`,
  })
})

app.get('/api/decks/:deckId/history', (req, res) => {
  if (req.params.deckId !== DECK_ID) {
    res.status(404).json({ error: 'Unknown deck' })
    return
  }
  res.json({ snapshots: listSnapshots(DECK_ID) })
})

app.post('/api/decks/:deckId/publish-snapshot', (req, res) => {
  if (req.params.deckId !== DECK_ID) {
    res.status(404).json({ error: 'Unknown deck' })
    return
  }
  const label = typeof req.body?.label === 'string' && req.body.label.trim().length > 0
    ? req.body.label.trim()
    : 'Snapshot'
  const autosave = Boolean(req.body?.autosave)
  const rawSnapshot = req.body?.snapshot
  const ydoc = getYDoc(DECK_ID)
  const snapshot = (rawSnapshot ?? getCurrentSnapshot(ydoc)) as DeckDocument
  const id = autosave ? 'autosave' : nanoid(12)
  const record = {
    id,
    label,
    createdAt: new Date().toISOString(),
    autosave,
  }
  saveSnapshot(DECK_ID, record, snapshot)
  res.status(201).json({ snapshot: record })
})

app.post('/api/decks/:deckId/restore-snapshot/:snapshotId', (req, res) => {
  if (req.params.deckId !== DECK_ID) {
    res.status(404).json({ error: 'Unknown deck' })
    return
  }
  const snapshot = loadSnapshot(DECK_ID, req.params.snapshotId)
  if (!snapshot) {
    res.status(404).json({ error: 'Snapshot not found' })
    return
  }
  const ydoc = getYDoc(DECK_ID)
  replaceWithSnapshot(ydoc, snapshot)
  res.json({ restored: true })
})

server.on('upgrade', (request, socket, head) => {
  const url = new URL(request.url ?? '/', `http://${request.headers.host}`)
  if (!url.pathname.startsWith('/collab/')) {
    socket.destroy()
    return
  }
  const roomId = decodeURIComponent(url.pathname.replace('/collab/', '') || DECK_ID)
  wss.handleUpgrade(request, socket, head, (ws: WebSocket, upgradeRequest: IncomingMessage = request) => {
    try {
      setupWSConnection(ws, upgradeRequest, { docName: roomId })
      ws.on('error', (error) => {
        console.error(`ws error ${roomId}`, error)
      })
    } catch (error) {
      console.error('ws setup failed', error)
      ws.close()
    }
  })
})

if (process.env.NODE_ENV === 'production' && path.join(__dirname, '../../app/dist')) {
  app.use(express.static(appDistDir))
  app.get('*', (req, res, next) => {
    if (req.path.startsWith('/api/') || req.path.startsWith('/collab/')) {
      next()
      return
    }
    res.sendFile(path.join(appDistDir, 'index.html'))
  })
}

server.listen(port, () => {
  console.log(`deck server listening on http://localhost:${port}`)
})
