import {
  DndContext,
  PointerSensor,
  closestCenter,
  useSensor,
  useSensors,
  type DragEndEvent,
} from '@dnd-kit/core'
import {
  SortableContext,
  useSortable,
  verticalListSortingStrategy,
} from '@dnd-kit/sortable'
import { CSS } from '@dnd-kit/utilities'
import clsx from 'clsx'
import Moveable from 'react-moveable'
import {
  type CSSProperties,
  type ChangeEvent,
  type PointerEvent as ReactPointerEvent,
  startTransition,
  useCallback,
  useEffect,
  useEffectEvent,
  useMemo,
  useRef,
  useState,
} from 'react'
import { WebsocketProvider } from 'y-websocket'
import * as Y from 'yjs'
import type {
  DeckDocument,
  ImageElement,
  ModelElement,
  ShapeElement,
  Slide,
  SlideElement,
  TableElement,
  TextElement,
} from '../../shared/src/deck.js'
import {
  SHARE_TOKEN,
  SLIDE_HEIGHT,
  SLIDE_WIDTH,
} from '../../shared/src/deck.js'
import './App.css'
import { readDeckDoc } from './lib/client-doc'
import {
  LOCAL_ORIGIN,
  type BootstrapPayload,
  type SnapshotMeta,
  createImageElement,
  createModelElement,
  createShapeElement,
  createTableElement,
  createTextElement,
  duplicateElements,
  getApiBaseUrl,
  getSocketBaseUrl,
  insertElement,
  nudgeElements,
  removeElements,
  reorderSlides,
  resolveAssetUrl,
  updateElementProps,
  updateLocalAwareness,
  updateTable,
  updateTextStyle,
  updateTextValue,
} from './lib/editor'

type PresencePeer = {
  clientId: number
  name: string
  color: string
  currentSlideId: string | null
  selectedElementIds: string[]
  editingTextId: string | null
}

type MarqueeState = {
  startX: number
  startY: number
  currentX: number
  currentY: number
}

type UploadIntent =
  | { kind: 'create-image' }
  | { kind: 'create-model' }
  | { kind: 'replace-image'; elementId: string }
  | { kind: 'replace-model'; elementId: string }

const colorPalette = ['#0a84ff', '#30d158', '#ff9f0a', '#bf5af2', '#ff453a', '#64d2ff']
const mobileQuery = '(max-width: 1023px)'

const coerceNumber = (value: string, fallback: number) => {
  const next = Number(value)
  return Number.isFinite(next) ? next : fallback
}

const formatTimestamp = (value: string) => {
  const date = new Date(value)
  return date.toLocaleString()
}

const isTextElement = (element: SlideElement | undefined | null): element is TextElement => element?.type === 'text'
const isImageElement = (element: SlideElement | undefined | null): element is ImageElement => element?.type === 'image'
const isModelElement = (element: SlideElement | undefined | null): element is ModelElement => element?.type === 'model'
const isShapeElement = (element: SlideElement | undefined | null): element is ShapeElement => element?.type === 'shape'
const isTableElement = (element: SlideElement | undefined | null): element is TableElement => element?.type === 'table'

const intersects = (marquee: MarqueeState, element: SlideElement) => {
  const left = Math.min(marquee.startX, marquee.currentX)
  const right = Math.max(marquee.startX, marquee.currentX)
  const top = Math.min(marquee.startY, marquee.currentY)
  const bottom = Math.max(marquee.startY, marquee.currentY)
  return !(
    element.x + element.w < left ||
    element.x > right ||
    element.y + element.h < top ||
    element.y > bottom
  )
}

const elementWrapperStyle = (element: SlideElement): CSSProperties => ({
  left: element.x,
  top: element.y,
  width: element.w,
  height: element.h,
  transform: `rotate(${element.rotation}deg)`,
  zIndex: element.zIndex,
})

const readPeers = (
  provider: WebsocketProvider | null,
  localClientId: number | null,
): PresencePeer[] => {
  if (!provider) {
    return []
  }
  const states = provider.awareness.getStates()
  return Array.from(states.entries())
    .filter(([clientId]) => clientId !== localClientId)
    .map(([clientId, state]) => ({
      clientId,
      name: String(state.user?.name ?? `Guest ${clientId}`),
      color: String(state.user?.color ?? '#0a84ff'),
      currentSlideId: (state.currentSlideId as string | null) ?? null,
      selectedElementIds: Array.isArray(state.selectedElementIds) ? state.selectedElementIds as string[] : [],
      editingTextId: (state.editingTextId as string | null) ?? null,
    }))
}

const SortableSlideCard = ({
  slide,
  isActive,
  onSelect,
  peerCount,
}: {
  slide: Slide
  isActive: boolean
  onSelect: () => void
  peerCount: number
}) => {
  const {
    attributes,
    listeners,
    setNodeRef,
    transform,
    transition,
    isDragging,
  } = useSortable({ id: slide.id })

  return (
    <button
      ref={setNodeRef}
      type="button"
      className={clsx('slide-card', isActive && 'slide-card-active', isDragging && 'slide-card-dragging')}
      style={{
        transform: CSS.Transform.toString(transform),
        transition,
      }}
      onClick={onSelect}
      {...attributes}
      {...listeners}
    >
      <span className="slide-card-index">{slide.id.replace('slide-', '')}</span>
      <span className="slide-card-name">{slide.name}</span>
      <span className="slide-card-meta">
        {slide.elements.length} elements
        {peerCount > 0 ? ` • ${peerCount} live` : ''}
      </span>
    </button>
  )
}

const renderTextPreview = (element: TextElement) => {
  const lines = element.text.split('\n').filter(Boolean)
  if (element.style.bullet) {
    return (
      <ul className="text-list">
        {lines.map((line, index) => (
          <li key={`${element.id}-${index}`}>{line}</li>
        ))}
      </ul>
    )
  }
  return (
    <div className="text-lines">
      {element.text.split('\n').map((line, index) => (
        <div key={`${element.id}-${index}`}>{line || '\u00a0'}</div>
      ))}
    </div>
  )
}

function App() {
  const [bootstrap, setBootstrap] = useState<BootstrapPayload | null>(null)
  const [deck, setDeck] = useState<DeckDocument | null>(null)
  const [connectionStatus, setConnectionStatus] = useState<'connecting' | 'connected' | 'disconnected'>('connecting')
  const [selectedSlideId, setSelectedSlideId] = useState<string>('slide-1')
  const [selectedElementIds, setSelectedElementIds] = useState<string[]>([])
  const [editingTextId, setEditingTextId] = useState<string | null>(null)
  const [zoom, setZoom] = useState(0.68)
  const [peers, setPeers] = useState<PresencePeer[]>([])
  const [history, setHistory] = useState<SnapshotMeta[]>([])
  const [collaboratorName, setCollaboratorName] = useState(() =>
    typeof window === 'undefined' ? '' : (window.localStorage.getItem('deck-collab-name') ?? ''),
  )
  const [collaboratorColor] = useState(() =>
    typeof window === 'undefined'
      ? colorPalette[0]
      : (window.localStorage.getItem('deck-collab-color') ?? colorPalette[Math.floor(Math.random() * colorPalette.length)]),
  )
  const [marquee, setMarquee] = useState<MarqueeState | null>(null)
  const [uploadIntent, setUploadIntent] = useState<UploadIntent | null>(null)
  const [isMobile, setIsMobile] = useState(false)

  const docRef = useRef<Y.Doc | null>(null)
  const providerRef = useRef<WebsocketProvider | null>(null)
  const undoManagerRef = useRef<Y.UndoManager | null>(null)
  const fileInputRef = useRef<HTMLInputElement | null>(null)
  const artboardRef = useRef<HTMLDivElement | null>(null)
  const autosaveTimer = useRef<number | null>(null)
  const latestDeckRef = useRef<DeckDocument | null>(null)
  const localClientIdRef = useRef<number | null>(null)

  const activeSlideId = useMemo(
    () => deck?.slides.some((slide) => slide.id === selectedSlideId) ? selectedSlideId : (deck?.slides[0]?.id ?? 'slide-1'),
    [deck, selectedSlideId],
  )

  const activeSlide = useMemo(
    () => deck?.slides.find((slide) => slide.id === activeSlideId) ?? deck?.slides[0] ?? null,
    [activeSlideId, deck],
  )

  const selectedElements = useMemo(
    () => activeSlide?.elements.filter((element) => selectedElementIds.includes(element.id)) ?? [],
    [activeSlide, selectedElementIds],
  )

  const selectedElement = selectedElements.length === 1 ? selectedElements[0] : null
  const selectedTargetSelector = selectedElement ? `[data-element-id="${selectedElement.id}"]` : null

  const peerSelections = useMemo(() => {
    const map = new Map<string, PresencePeer[]>()
    peers
      .filter((peer) => peer.currentSlideId === activeSlide?.id)
      .forEach((peer) => {
        peer.selectedElementIds.forEach((elementId) => {
          const list = map.get(elementId) ?? []
          list.push(peer)
          map.set(elementId, list)
        })
      })
    return map
  }, [activeSlide?.id, peers])

  const lockedByPeer = useMemo(() => {
    const map = new Map<string, PresencePeer>()
    if (!activeSlide) {
      return map
    }
    activeSlide.elements.forEach((element) => {
      if (element.type === 'text') {
        return
      }
      const contender = peerSelections.get(element.id)?.[0]
      if (contender) {
        map.set(element.id, contender)
      }
    })
    return map
  }, [activeSlide, peerSelections])

  const isLockedSelection = selectedElement ? lockedByPeer.has(selectedElement.id) : false
  const canEdit = !isMobile

  const refreshHistory = useCallback(async (deckId: string) => {
    const response = await fetch(`${getApiBaseUrl()}/api/decks/${deckId}/history`)
    if (!response.ok) {
      return
    }
    const payload = await response.json() as { snapshots: SnapshotMeta[] }
    setHistory(payload.snapshots)
  }, [])

  const syncDeck = useEffectEvent((doc: Y.Doc) => {
    startTransition(() => {
      const nextDeck = readDeckDoc(doc)
      latestDeckRef.current = nextDeck
      setDeck(nextDeck)
    })
  })

  useEffect(() => {
    const media = window.matchMedia(mobileQuery)
    const apply = () => setIsMobile(media.matches)
    apply()
    media.addEventListener('change', apply)
    return () => media.removeEventListener('change', apply)
  }, [])

  useEffect(() => {
    window.localStorage.setItem('deck-collab-name', collaboratorName)
  }, [collaboratorName])

  useEffect(() => {
    window.localStorage.setItem('deck-collab-color', collaboratorColor)
  }, [collaboratorColor])

  useEffect(() => {
    let disposed = false

    const connect = async () => {
      const response = await fetch(`${getApiBaseUrl()}/api/share/${SHARE_TOKEN}/bootstrap`)
      const payload = await response.json() as BootstrapPayload
      if (disposed) {
        return
      }
      setBootstrap(payload)
      setCollaboratorName((current) => current || payload.collaboratorLabel)

      const doc = new Y.Doc()
      docRef.current = doc
      const provider = new WebsocketProvider(`${getSocketBaseUrl()}${payload.websocketPath}`, payload.roomId, doc, {
        connect: true,
        params: { share: payload.shareToken },
      })
      providerRef.current = provider
      localClientIdRef.current = provider.awareness.clientID

      undoManagerRef.current = new Y.UndoManager([doc.getArray('slides')], {
        trackedOrigins: new Set([LOCAL_ORIGIN]),
      })

      provider.on('status', (event) => {
        setConnectionStatus(event.status)
      })

      provider.on('sync', () => {
        syncDeck(doc)
      })

      doc.on('update', () => {
        syncDeck(doc)
      })

      const awarenessChange = () => {
        setPeers(readPeers(provider, localClientIdRef.current))
      }

      provider.awareness.on('change', awarenessChange)
      awarenessChange()
      syncDeck(doc)
      await refreshHistory(payload.deckId)
    }

    void connect()

    return () => {
      disposed = true
      providerRef.current?.destroy()
      providerRef.current = null
      docRef.current?.destroy()
      docRef.current = null
    }
  }, [refreshHistory])

  useEffect(() => {
    const awareness = providerRef.current?.awareness
    if (!awareness) {
      return
    }
    updateLocalAwareness(awareness, {
      name: collaboratorName || bootstrap?.collaboratorLabel || 'Guest',
      color: collaboratorColor,
      currentSlideId: activeSlide?.id ?? null,
      selectedElementIds,
      editingTextId,
    })
  }, [
    activeSlide?.id,
    bootstrap?.collaboratorLabel,
    collaboratorColor,
    collaboratorName,
    editingTextId,
    selectedElementIds,
  ])

  useEffect(() => {
    if (!deck || !bootstrap) {
      return
    }
    if (autosaveTimer.current) {
      window.clearTimeout(autosaveTimer.current)
    }
    autosaveTimer.current = window.setTimeout(() => {
      void fetch(`${getApiBaseUrl()}/api/decks/${bootstrap.deckId}/publish-snapshot`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          label: 'Autosave',
          autosave: true,
          snapshot: latestDeckRef.current,
        }),
      }).then(() => refreshHistory(bootstrap.deckId))
    }, 30_000)

    return () => {
      if (autosaveTimer.current) {
        window.clearTimeout(autosaveTimer.current)
        autosaveTimer.current = null
      }
    }
  }, [bootstrap, deck, refreshHistory])

  useEffect(() => {
    const beforeUnload = () => {
      if (!bootstrap || !latestDeckRef.current) {
        return
      }
      navigator.sendBeacon(
        `${getApiBaseUrl()}/api/decks/${bootstrap.deckId}/publish-snapshot`,
        new Blob(
          [
            JSON.stringify({
              label: 'Autosave',
              autosave: true,
              snapshot: latestDeckRef.current,
            }),
          ],
          { type: 'application/json' },
        ),
      )
    }
    window.addEventListener('beforeunload', beforeUnload)
    return () => window.removeEventListener('beforeunload', beforeUnload)
  }, [bootstrap])

  const handleGlobalKeydown = useEffectEvent((event: KeyboardEvent) => {
    if (!activeSlide || !docRef.current || !canEdit) {
      return
    }
    const activeTag = (document.activeElement as HTMLElement | null)?.tagName
    const isTyping = activeTag === 'INPUT' || activeTag === 'TEXTAREA'

    if (editingTextId && event.key === 'Escape') {
      setEditingTextId(null)
      return
    }

    if (!isTyping && selectedElementIds.length > 0) {
      if (event.key === 'Backspace' || event.key === 'Delete') {
        event.preventDefault()
        removeElements(docRef.current, activeSlide.id, selectedElementIds)
        setSelectedElementIds([])
        setEditingTextId(null)
      } else if (event.key === 'd' && (event.metaKey || event.ctrlKey)) {
        event.preventDefault()
        const ids = duplicateElements(docRef.current, activeSlide.id, selectedElements)
        setSelectedElementIds(ids)
      } else if (event.key.startsWith('Arrow')) {
        const delta = event.shiftKey ? 10 : 1
        const dx = event.key === 'ArrowLeft' ? -delta : event.key === 'ArrowRight' ? delta : 0
        const dy = event.key === 'ArrowUp' ? -delta : event.key === 'ArrowDown' ? delta : 0
        if (dx || dy) {
          event.preventDefault()
          nudgeElements(docRef.current, activeSlide.id, selectedElementIds, dx, dy)
        }
      }
    }

    if (!isTyping && (event.metaKey || event.ctrlKey) && event.key === 'z') {
      event.preventDefault()
      if (event.shiftKey) {
        undoManagerRef.current?.redo()
      } else {
        undoManagerRef.current?.undo()
      }
    }
  })

  useEffect(() => {
    window.addEventListener('keydown', handleGlobalKeydown)
    return () => window.removeEventListener('keydown', handleGlobalKeydown)
  }, [])

  const handleSlideSortEnd = (event: DragEndEvent) => {
    if (!docRef.current || !event.over || event.active.id === event.over.id) {
      return
    }
    reorderSlides(docRef.current, String(event.active.id), String(event.over.id))
  }

  const sensors = useSensors(useSensor(PointerSensor, { activationConstraint: { distance: 6 } }))

  const handleMarqueeStart = (event: ReactPointerEvent<HTMLDivElement>) => {
    if (!activeSlide || !canEdit || editingTextId) {
      return
    }
    if (event.target !== artboardRef.current) {
      return
    }
    const rect = artboardRef.current?.getBoundingClientRect()
    if (!rect) {
      return
    }
    const x = (event.clientX - rect.left) / zoom
    const y = (event.clientY - rect.top) / zoom
    setSelectedElementIds([])
    setEditingTextId(null)
    setMarquee({
      startX: x,
      startY: y,
      currentX: x,
      currentY: y,
    })
  }

  const handleMarqueeMove = (event: ReactPointerEvent<HTMLDivElement>) => {
    if (!marquee || !artboardRef.current) {
      return
    }
    const rect = artboardRef.current.getBoundingClientRect()
    setMarquee({
      ...marquee,
      currentX: (event.clientX - rect.left) / zoom,
      currentY: (event.clientY - rect.top) / zoom,
    })
  }

  const handleMarqueeEnd = () => {
    if (!marquee || !activeSlide) {
      setMarquee(null)
      return
    }
    const width = Math.abs(marquee.currentX - marquee.startX)
    const height = Math.abs(marquee.currentY - marquee.startY)
    if (width > 4 || height > 4) {
      const hits = activeSlide.elements
        .filter((element) => intersects(marquee, element))
        .map((element) => element.id)
      setSelectedElementIds(hits)
    }
    setMarquee(null)
  }

  const uploadFile = async (file: File) => {
    if (!bootstrap) {
      return null
    }
    const formData = new FormData()
    formData.append('file', file)
    const response = await fetch(`${getApiBaseUrl()}/api/decks/${bootstrap.deckId}/assets`, {
      method: 'POST',
      body: formData,
    })
    if (!response.ok) {
      return null
    }
    return await response.json() as { url: string; kind: 'image' | 'model' }
  }

  const handleUploadSelection = async (event: ChangeEvent<HTMLInputElement>) => {
    const file = event.target.files?.[0]
    if (!file || !uploadIntent || !activeSlide || !docRef.current) {
      return
    }
    const result = await uploadFile(file)
    if (!result) {
      return
    }
    if (uploadIntent.kind === 'create-image') {
      const element = createImageElement(result.url, file.name)
      insertElement(docRef.current, activeSlide.id, element)
      setSelectedElementIds([element.id])
    } else if (uploadIntent.kind === 'create-model') {
      const element = createModelElement(result.url, '', file.name)
      insertElement(docRef.current, activeSlide.id, element)
      setSelectedElementIds([element.id])
    } else if (uploadIntent.kind === 'replace-image') {
      updateElementProps(docRef.current, activeSlide.id, uploadIntent.elementId, {
        src: result.url,
        alt: file.name,
      })
    } else if (uploadIntent.kind === 'replace-model') {
      updateElementProps(docRef.current, activeSlide.id, uploadIntent.elementId, {
        src: result.url,
      })
    }
    setUploadIntent(null)
    event.target.value = ''
  }

  const queueUpload = (intent: UploadIntent) => {
    setUploadIntent(intent)
    fileInputRef.current?.click()
  }

  const addElement = (kind: 'text' | 'shape' | 'table') => {
    if (!docRef.current || !activeSlide) {
      return
    }
    const element =
      kind === 'text'
        ? createTextElement()
        : kind === 'shape'
          ? createShapeElement()
          : createTableElement()
    insertElement(docRef.current, activeSlide.id, element)
    setSelectedElementIds([element.id])
    if (element.type === 'text') {
      setEditingTextId(element.id)
    }
  }

  const publishSnapshot = async () => {
    if (!bootstrap || !latestDeckRef.current) {
      return
    }
    const label = window.prompt('Snapshot label', `Manual snapshot ${new Date().toLocaleTimeString()}`)
    if (!label) {
      return
    }
    await fetch(`${getApiBaseUrl()}/api/decks/${bootstrap.deckId}/publish-snapshot`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        label,
        autosave: false,
        snapshot: latestDeckRef.current,
      }),
    })
    await refreshHistory(bootstrap.deckId)
  }

  const restoreSnapshot = async (snapshotId: string) => {
    if (!bootstrap) {
      return
    }
    const confirmed = window.confirm('Restore this snapshot for everyone currently in the deck?')
    if (!confirmed) {
      return
    }
    await fetch(`${getApiBaseUrl()}/api/decks/${bootstrap.deckId}/restore-snapshot/${snapshotId}`, {
      method: 'POST',
    })
    setSelectedElementIds([])
    setEditingTextId(null)
    await refreshHistory(bootstrap.deckId)
  }

  const selectedTableText = isTableElement(selectedElement)
    ? `${selectedElement.columns.join('\t')}\n${selectedElement.rows.map((row) => row.join('\t')).join('\n')}`
    : ''

  if (!deck || !bootstrap) {
    return (
      <div className="app-shell app-loading">
        <div className="loading-card">
          <div className="loading-title">Collaborative deck editor</div>
          <div className="loading-copy">Connecting to the shared presentation…</div>
        </div>
      </div>
    )
  }

  return (
    <div className="app-shell">
      <aside className="slide-rail">
        <div className="rail-header">
          <div>
            <div className="eyebrow-label">Deck</div>
            <h1>BMS Collaborative Slides</h1>
          </div>
          <div className={clsx('status-pill', connectionStatus)}>{connectionStatus}</div>
        </div>
        <div className="rail-copy">
          Anyone with the share link can edit. Drag slides to reorder. Remote presence is shown in the rail and on the canvas.
        </div>
        <DndContext sensors={sensors} collisionDetection={closestCenter} onDragEnd={handleSlideSortEnd}>
          <SortableContext items={deck.slides.map((slide) => slide.id)} strategy={verticalListSortingStrategy}>
            <div className="slide-card-list">
              {deck.slides.map((slide) => (
                <SortableSlideCard
                  key={slide.id}
                  slide={slide}
                  isActive={slide.id === activeSlide?.id}
                  onSelect={() => {
                    setSelectedSlideId(slide.id)
                    setSelectedElementIds([])
                    setEditingTextId(null)
                  }}
                  peerCount={peers.filter((peer) => peer.currentSlideId === slide.id).length}
                />
              ))}
            </div>
          </SortableContext>
        </DndContext>
      </aside>

      <main className="workspace">
        <header className="toolbar">
          <div className="toolbar-group">
            <button type="button" className="toolbar-button primary" disabled={!canEdit} onClick={() => addElement('text')}>
              Add text
            </button>
            <button type="button" className="toolbar-button" disabled={!canEdit} onClick={() => queueUpload({ kind: 'create-image' })}>
              Add image
            </button>
            <button type="button" className="toolbar-button" disabled={!canEdit} onClick={() => queueUpload({ kind: 'create-model' })}>
              Add model
            </button>
            <button type="button" className="toolbar-button" disabled={!canEdit} onClick={() => addElement('shape')}>
              Add shape
            </button>
            <button type="button" className="toolbar-button" disabled={!canEdit} onClick={() => addElement('table')}>
              Add table
            </button>
          </div>

          <div className="toolbar-group">
            <button type="button" className="toolbar-button" disabled={!canEdit || !selectedElements.length} onClick={() => {
              if (!docRef.current || !activeSlide) {
                return
              }
              const ids = duplicateElements(docRef.current, activeSlide.id, selectedElements)
              setSelectedElementIds(ids)
            }}>
              Duplicate
            </button>
            <button type="button" className="toolbar-button" disabled={!canEdit || !selectedElements.length} onClick={() => {
              if (!docRef.current || !activeSlide) {
                return
              }
              removeElements(docRef.current, activeSlide.id, selectedElementIds)
              setSelectedElementIds([])
            }}>
              Delete
            </button>
            <button type="button" className="toolbar-button" disabled={!canEdit} onClick={() => undoManagerRef.current?.undo()}>
              Undo
            </button>
            <button type="button" className="toolbar-button" disabled={!canEdit} onClick={() => undoManagerRef.current?.redo()}>
              Redo
            </button>
            <button type="button" className="toolbar-button" onClick={() => setZoom((value) => Math.max(0.35, Number((value - 0.05).toFixed(2))))}>
              -
            </button>
            <div className="zoom-readout">{Math.round(zoom * 100)}%</div>
            <button type="button" className="toolbar-button" onClick={() => setZoom((value) => Math.min(1.1, Number((value + 0.05).toFixed(2))))}>
              +
            </button>
            <button type="button" className="toolbar-button primary" disabled={!canEdit} onClick={publishSnapshot}>
              Publish snapshot
            </button>
          </div>

          <div className="toolbar-group toolbar-presence">
            <input
              className="name-input"
              value={collaboratorName}
              onChange={(event) => setCollaboratorName(event.target.value)}
              placeholder="Your name"
            />
            <div className="presence-list">
              <span className="presence-pill local" style={{ borderColor: collaboratorColor }}>
                <span className="presence-dot" style={{ background: collaboratorColor }} />
                {collaboratorName || bootstrap.collaboratorLabel}
              </span>
              {peers.map((peer) => (
                <span key={peer.clientId} className="presence-pill" style={{ borderColor: peer.color }}>
                  <span className="presence-dot" style={{ background: peer.color }} />
                  {peer.name}
                </span>
              ))}
            </div>
          </div>
        </header>

        <section className="editor-area">
          <div className="canvas-panel">
            {isMobile && (
              <div className="mobile-banner">
                Mobile is view-only in v1. Open this deck on desktop to edit collaboratively.
              </div>
            )}
            <div className="canvas-stage">
              <div
                ref={artboardRef}
                className="slide-canvas"
                style={{
                  width: SLIDE_WIDTH,
                  height: SLIDE_HEIGHT,
                  transform: `scale(${zoom})`,
                  background: `linear-gradient(135deg, ${activeSlide?.background.start} 0%, ${activeSlide?.background.mid} 42%, ${activeSlide?.background.end} 100%)`,
                }}
                onPointerDown={handleMarqueeStart}
                onPointerMove={handleMarqueeMove}
                onPointerUp={handleMarqueeEnd}
                onPointerLeave={handleMarqueeEnd}
              >
                <div className="canvas-grid" />
                {activeSlide?.elements
                  .slice()
                  .sort((left, right) => left.zIndex - right.zIndex)
                  .map((element) => {
                    const peerLock = lockedByPeer.get(element.id)
                    const peerBadges = peerSelections.get(element.id) ?? []
                    const selected = selectedElementIds.includes(element.id)
                    const resolvedImage = element.type === 'image' || element.type === 'model'
                      ? resolveAssetUrl(element.src, bootstrap.assetBaseUrl)
                      : ''
                    const resolvedPoster = element.type === 'model'
                      ? resolveAssetUrl(element.poster, bootstrap.assetBaseUrl)
                      : ''
                    const textStyle = isTextElement(element)
                      ? {
                          fontFamily: element.style.fontFamily,
                          fontSize: element.style.fontSize,
                          fontWeight: element.style.fontWeight,
                          fontStyle: element.style.italic ? 'italic' : 'normal',
                          color: element.style.color,
                          textAlign: element.style.align,
                          lineHeight: element.style.lineHeight,
                          letterSpacing: element.style.letterSpacing,
                          textTransform: element.style.uppercase ? 'uppercase' : 'none',
                          background: element.style.background,
                          borderColor: element.style.borderColor,
                          borderWidth: element.style.borderWidth,
                          borderRadius: element.style.radius,
                          padding: element.style.padding,
                          boxShadow: element.style.shadow,
                        }
                      : undefined

                    return (
                      <div
                        key={element.id}
                        data-element-id={element.id}
                        className={clsx(
                          'slide-element',
                          selected && 'slide-element-selected',
                          peerLock && !selected && 'slide-element-locked',
                        )}
                        style={elementWrapperStyle(element)}
                        onPointerDown={(event) => {
                          event.stopPropagation()
                          if (!canEdit) {
                            return
                          }
                          if (event.shiftKey || event.metaKey || event.ctrlKey) {
                            setSelectedElementIds((current) =>
                              current.includes(element.id)
                                ? current.filter((id) => id !== element.id)
                                : [...current, element.id],
                            )
                          } else {
                            setSelectedElementIds([element.id])
                          }
                          if (element.type !== 'text') {
                            setEditingTextId(null)
                          }
                        }}
                        onDoubleClick={() => {
                          if (canEdit && element.type === 'text') {
                            setSelectedElementIds([element.id])
                            setEditingTextId(element.id)
                          }
                        }}
                      >
                        {isShapeElement(element) && (
                          <div
                            className="shape-element"
                            style={{
                              background: element.fill,
                              borderColor: element.borderColor,
                              borderWidth: element.borderWidth,
                              borderRadius: element.radius,
                              opacity: element.opacity,
                              boxShadow: element.shadow,
                            }}
                          />
                        )}
                        {isTextElement(element) && (
                          editingTextId === element.id && canEdit ? (
                            <textarea
                              autoFocus
                              className="inline-textarea"
                              value={element.text}
                              style={textStyle}
                              onChange={(event) => {
                                if (!docRef.current || !activeSlide) {
                                  return
                                }
                                updateTextValue(docRef.current, activeSlide.id, element.id, event.target.value)
                              }}
                              onBlur={() => setEditingTextId(null)}
                            />
                          ) : (
                            <div className="text-element" style={textStyle}>
                              {renderTextPreview(element)}
                            </div>
                          )
                        )}
                        {isImageElement(element) && (
                          <div
                            className="media-shell"
                            style={{
                              background: element.background,
                              borderColor: element.borderColor,
                              borderWidth: element.borderWidth,
                              borderRadius: element.radius,
                              boxShadow: element.shadow,
                            }}
                          >
                            <img
                              className="media-frame"
                              src={resolvedImage}
                              alt={element.alt}
                              style={{ objectFit: element.fit }}
                            />
                            {element.caption && <div className="media-caption">{element.caption}</div>}
                          </div>
                        )}
                        {isModelElement(element) && (
                          <div
                            className="media-shell"
                            style={{
                              background: element.background,
                              borderColor: element.borderColor,
                              borderWidth: element.borderWidth,
                              borderRadius: element.radius,
                              boxShadow: element.shadow,
                            }}
                          >
                            {/* model-viewer is loaded globally in index.html */}
                            <model-viewer
                              className="model-viewer-frame"
                              src={resolvedImage}
                              poster={resolvedPoster}
                              auto-rotate={element.autoRotate ? '' : undefined}
                              camera-controls={element.cameraControls ? '' : undefined}
                              style={{ objectFit: element.fit } as CSSProperties}
                            />
                            {element.caption && <div className="media-caption">{element.caption}</div>}
                          </div>
                        )}
                        {isTableElement(element) && (
                          <div className="table-shell">
                            <table
                              className="slide-table"
                              style={{
                                borderColor: element.style.borderColor,
                                fontSize: element.style.fontSize,
                              }}
                            >
                              <thead>
                                <tr>
                                  {element.columns.map((column) => (
                                    <th
                                      key={`${element.id}-${column}`}
                                      style={{
                                        background: element.style.headerBackground,
                                        color: element.style.headerColor,
                                      }}
                                    >
                                      {column}
                                    </th>
                                  ))}
                                </tr>
                              </thead>
                              <tbody>
                                {element.rows.map((row, rowIndex) => (
                                  <tr key={`${element.id}-row-${rowIndex}`}>
                                    {row.map((cell, cellIndex) => (
                                      <td
                                        key={`${element.id}-${rowIndex}-${cellIndex}`}
                                        style={{
                                          background: element.style.cellBackground,
                                          color: element.style.cellColor,
                                        }}
                                      >
                                        {cell}
                                      </td>
                                    ))}
                                  </tr>
                                ))}
                              </tbody>
                            </table>
                          </div>
                        )}
                        {peerBadges.length > 0 && (
                          <div className="remote-badges">
                            {peerBadges.map((peer) => (
                              <span key={`${element.id}-${peer.clientId}`} className="remote-badge" style={{ borderColor: peer.color }}>
                                <span className="presence-dot" style={{ background: peer.color }} />
                                {peer.name}
                              </span>
                            ))}
                          </div>
                        )}
                      </div>
                    )
                  })}
                {marquee && (
                  <div
                    className="marquee-box"
                    style={{
                      left: Math.min(marquee.startX, marquee.currentX),
                      top: Math.min(marquee.startY, marquee.currentY),
                      width: Math.abs(marquee.currentX - marquee.startX),
                      height: Math.abs(marquee.currentY - marquee.startY),
                    }}
                  />
                )}
              </div>

              {selectedTargetSelector && selectedElement && !editingTextId && canEdit && (
                <Moveable
                  target={selectedTargetSelector}
                  draggable={!isLockedSelection}
                  resizable={!isLockedSelection}
                  rotatable={!isLockedSelection}
                  snappable
                  keepRatio={
                    isImageElement(selectedElement) || isModelElement(selectedElement)
                      ? selectedElement.preserveAspectRatio
                      : false
                  }
                  bounds={{
                    left: 0,
                    top: 0,
                    right: SLIDE_WIDTH,
                    bottom: SLIDE_HEIGHT,
                  }}
                  renderDirections={['nw', 'ne', 'sw', 'se', 'n', 'e', 's', 'w']}
                  onDrag={({ target, left, top }) => {
                    target.style.left = `${left}px`
                    target.style.top = `${top}px`
                  }}
                  onDragEnd={({ target }) => {
                    if (!docRef.current || !activeSlide || !selectedElement) {
                      return
                    }
                    updateElementProps(docRef.current, activeSlide.id, selectedElement.id, {
                      x: Number.parseFloat(target.style.left),
                      y: Number.parseFloat(target.style.top),
                    })
                  }}
                  onResize={({ target, width, height, drag }) => {
                    target.style.width = `${width}px`
                    target.style.height = `${height}px`
                    target.style.left = `${drag.left}px`
                    target.style.top = `${drag.top}px`
                  }}
                  onResizeEnd={({ target }) => {
                    if (!docRef.current || !activeSlide || !selectedElement) {
                      return
                    }
                    updateElementProps(docRef.current, activeSlide.id, selectedElement.id, {
                      x: Number.parseFloat(target.style.left),
                      y: Number.parseFloat(target.style.top),
                      w: Number.parseFloat(target.style.width),
                      h: Number.parseFloat(target.style.height),
                    })
                  }}
                  onRotate={({ target, beforeRotate }) => {
                    target.style.transform = `rotate(${beforeRotate}deg)`
                  }}
                  onRotateEnd={({ target }) => {
                    if (!docRef.current || !activeSlide || !selectedElement) {
                      return
                    }
                    const match = target.style.transform.match(/rotate\(([-\d.]+)deg\)/)
                    updateElementProps(docRef.current, activeSlide.id, selectedElement.id, {
                      rotation: match ? Number.parseFloat(match[1]) : selectedElement.rotation,
                    })
                  }}
                />
              )}
            </div>
          </div>

          <aside className="inspector">
            <div className="inspector-section">
              <div className="eyebrow-label">Slide</div>
              <label className="field">
                <span>Name</span>
                <input
                  value={activeSlide?.name ?? ''}
                  onChange={(event) => {
                    const next = event.target.value
                    if (!docRef.current || !activeSlide || !canEdit) {
                      return
                    }
                    const slideMap = docRef.current.getArray<Y.Map<unknown>>('slides').toArray().find((slide) => slide.get('id') === activeSlide.id)
                    if (slideMap) {
                      docRef.current.transact(() => {
                        slideMap.set('name', next)
                      }, LOCAL_ORIGIN)
                    }
                  }}
                  disabled={!canEdit}
                />
              </label>
              <div className="slide-summary">
                {activeSlide?.elements.length ?? 0} editable layers on a {SLIDE_WIDTH} × {SLIDE_HEIGHT} canvas.
              </div>
            </div>

            {selectedElement ? (
              <>
                <div className="inspector-section">
                  <div className="eyebrow-label">Selection</div>
                  <div className="selection-title">{selectedElement.name}</div>
                  {lockedByPeer.get(selectedElement.id) && (
                    <div className="lock-banner">
                      Locked for transforms by {lockedByPeer.get(selectedElement.id)?.name}
                    </div>
                  )}
                  <div className="field-grid">
                    <label className="field">
                      <span>X</span>
                      <input
                        type="number"
                        value={Math.round(selectedElement.x)}
                        disabled={!canEdit}
                        onChange={(event) => docRef.current && activeSlide && updateElementProps(docRef.current, activeSlide.id, selectedElement.id, {
                          x: coerceNumber(event.target.value, selectedElement.x),
                        })}
                      />
                    </label>
                    <label className="field">
                      <span>Y</span>
                      <input
                        type="number"
                        value={Math.round(selectedElement.y)}
                        disabled={!canEdit}
                        onChange={(event) => docRef.current && activeSlide && updateElementProps(docRef.current, activeSlide.id, selectedElement.id, {
                          y: coerceNumber(event.target.value, selectedElement.y),
                        })}
                      />
                    </label>
                    <label className="field">
                      <span>W</span>
                      <input
                        type="number"
                        value={Math.round(selectedElement.w)}
                        disabled={!canEdit}
                        onChange={(event) => docRef.current && activeSlide && updateElementProps(docRef.current, activeSlide.id, selectedElement.id, {
                          w: coerceNumber(event.target.value, selectedElement.w),
                        })}
                      />
                    </label>
                    <label className="field">
                      <span>H</span>
                      <input
                        type="number"
                        value={Math.round(selectedElement.h)}
                        disabled={!canEdit}
                        onChange={(event) => docRef.current && activeSlide && updateElementProps(docRef.current, activeSlide.id, selectedElement.id, {
                          h: coerceNumber(event.target.value, selectedElement.h),
                        })}
                      />
                    </label>
                  </div>
                </div>

                {isTextElement(selectedElement) && (
                  <div className="inspector-section">
                    <div className="eyebrow-label">Text</div>
                    <div className="field-grid">
                      <label className="field">
                        <span>Size</span>
                        <input
                          type="number"
                          value={selectedElement.style.fontSize}
                          disabled={!canEdit}
                          onChange={(event) => docRef.current && activeSlide && updateTextStyle(docRef.current, activeSlide.id, selectedElement.id, {
                            fontSize: coerceNumber(event.target.value, selectedElement.style.fontSize),
                          })}
                        />
                      </label>
                      <label className="field">
                        <span>Color</span>
                        <input
                          type="color"
                          value={selectedElement.style.color}
                          disabled={!canEdit}
                          onChange={(event) => docRef.current && activeSlide && updateTextStyle(docRef.current, activeSlide.id, selectedElement.id, {
                            color: event.target.value,
                          })}
                        />
                      </label>
                      <label className="field">
                        <span>Align</span>
                        <select
                          value={selectedElement.style.align}
                          disabled={!canEdit}
                          onChange={(event) => docRef.current && activeSlide && updateTextStyle(docRef.current, activeSlide.id, selectedElement.id, {
                            align: event.target.value as TextElement['style']['align'],
                          })}
                        >
                          <option value="left">Left</option>
                          <option value="center">Center</option>
                          <option value="right">Right</option>
                        </select>
                      </label>
                      <label className="field">
                        <span>Weight</span>
                        <input
                          type="number"
                          value={selectedElement.style.fontWeight}
                          disabled={!canEdit}
                          onChange={(event) => docRef.current && activeSlide && updateTextStyle(docRef.current, activeSlide.id, selectedElement.id, {
                            fontWeight: coerceNumber(event.target.value, selectedElement.style.fontWeight),
                          })}
                        />
                      </label>
                    </div>
                    <div className="toggle-row">
                      <button
                        type="button"
                        className={clsx('toggle-button', selectedElement.style.italic && 'toggle-button-active')}
                        disabled={!canEdit}
                        onClick={() => docRef.current && activeSlide && updateTextStyle(docRef.current, activeSlide.id, selectedElement.id, {
                          italic: !selectedElement.style.italic,
                        })}
                      >
                        Italic
                      </button>
                      <button
                        type="button"
                        className={clsx('toggle-button', selectedElement.style.bullet && 'toggle-button-active')}
                        disabled={!canEdit}
                        onClick={() => docRef.current && activeSlide && updateTextStyle(docRef.current, activeSlide.id, selectedElement.id, {
                          bullet: !selectedElement.style.bullet,
                        })}
                      >
                        Bullets
                      </button>
                      <button
                        type="button"
                        className={clsx('toggle-button', selectedElement.style.uppercase && 'toggle-button-active')}
                        disabled={!canEdit}
                        onClick={() => docRef.current && activeSlide && updateTextStyle(docRef.current, activeSlide.id, selectedElement.id, {
                          uppercase: !selectedElement.style.uppercase,
                        })}
                      >
                        Uppercase
                      </button>
                    </div>
                    <button
                      type="button"
                      className="toolbar-button full-width"
                      disabled={!canEdit}
                      onClick={() => setEditingTextId(selectedElement.id)}
                    >
                      Edit text inline
                    </button>
                  </div>
                )}

                {(isImageElement(selectedElement) || isModelElement(selectedElement)) && (
                  <div className="inspector-section">
                    <div className="eyebrow-label">Asset</div>
                    <label className="field">
                      <span>Caption</span>
                      <textarea
                        value={selectedElement.caption}
                        disabled={!canEdit}
                        onChange={(event) => docRef.current && activeSlide && updateElementProps(docRef.current, activeSlide.id, selectedElement.id, {
                          caption: event.target.value,
                        })}
                      />
                    </label>
                    <div className="field-grid">
                      <label className="field">
                        <span>Fit</span>
                        <select
                          value={selectedElement.fit}
                          disabled={!canEdit}
                          onChange={(event) => docRef.current && activeSlide && updateElementProps(docRef.current, activeSlide.id, selectedElement.id, {
                            fit: event.target.value,
                          })}
                        >
                          <option value="contain">Contain</option>
                          <option value="cover">Cover</option>
                        </select>
                      </label>
                      <label className="field">
                        <span>Radius</span>
                        <input
                          type="number"
                          value={selectedElement.radius}
                          disabled={!canEdit}
                          onChange={(event) => docRef.current && activeSlide && updateElementProps(docRef.current, activeSlide.id, selectedElement.id, {
                            radius: coerceNumber(event.target.value, selectedElement.radius),
                          })}
                        />
                      </label>
                    </div>
                    <div className="toggle-row">
                      <button
                        type="button"
                        className={clsx('toggle-button', selectedElement.preserveAspectRatio && 'toggle-button-active')}
                        disabled={!canEdit}
                        onClick={() => docRef.current && activeSlide && updateElementProps(docRef.current, activeSlide.id, selectedElement.id, {
                          preserveAspectRatio: !selectedElement.preserveAspectRatio,
                        })}
                      >
                        Lock ratio
                      </button>
                      {isModelElement(selectedElement) && (
                        <button
                          type="button"
                          className={clsx('toggle-button', selectedElement.autoRotate && 'toggle-button-active')}
                          disabled={!canEdit}
                          onClick={() => docRef.current && activeSlide && updateElementProps(docRef.current, activeSlide.id, selectedElement.id, {
                            autoRotate: !selectedElement.autoRotate,
                          })}
                        >
                          Auto rotate
                        </button>
                      )}
                    </div>
                    <button
                      type="button"
                      className="toolbar-button full-width"
                      disabled={!canEdit}
                      onClick={() => queueUpload({ kind: isImageElement(selectedElement) ? 'replace-image' : 'replace-model', elementId: selectedElement.id })}
                    >
                      Replace asset
                    </button>
                  </div>
                )}

                {isShapeElement(selectedElement) && (
                  <div className="inspector-section">
                    <div className="eyebrow-label">Shape</div>
                    <div className="field-grid">
                      <label className="field">
                        <span>Fill</span>
                        <input
                          value={selectedElement.fill}
                          disabled={!canEdit}
                          onChange={(event) => docRef.current && activeSlide && updateElementProps(docRef.current, activeSlide.id, selectedElement.id, {
                            fill: event.target.value,
                          })}
                        />
                      </label>
                      <label className="field">
                        <span>Opacity</span>
                        <input
                          type="number"
                          step="0.05"
                          min="0"
                          max="1"
                          value={selectedElement.opacity}
                          disabled={!canEdit}
                          onChange={(event) => docRef.current && activeSlide && updateElementProps(docRef.current, activeSlide.id, selectedElement.id, {
                            opacity: coerceNumber(event.target.value, selectedElement.opacity),
                          })}
                        />
                      </label>
                    </div>
                  </div>
                )}

                {isTableElement(selectedElement) && (
                  <div className="inspector-section">
                    <div className="eyebrow-label">Table</div>
                    <label className="field">
                      <span>Columns + Rows (tab separated)</span>
                      <textarea
                        className="table-editor"
                        value={selectedTableText}
                        disabled={!canEdit}
                        onChange={(event) => {
                          if (!docRef.current || !activeSlide) {
                            return
                          }
                          const lines = event.target.value.split('\n')
                          const [headerLine, ...bodyLines] = lines
                          const columns = headerLine.split('\t')
                          const rows = bodyLines.filter((line) => line.length > 0).map((line) => line.split('\t'))
                          updateTable(docRef.current, activeSlide.id, selectedElement.id, { columns, rows })
                        }}
                      />
                    </label>
                  </div>
                )}
              </>
            ) : (
              <div className="inspector-section">
                <div className="eyebrow-label">Selection</div>
                <div className="selection-title">No layer selected</div>
                <div className="slide-summary">
                  Click a layer to edit it. Double-click text to type directly on the slide.
                </div>
              </div>
            )}

            <div className="inspector-section">
              <div className="eyebrow-label">Snapshots</div>
              <div className="snapshot-list">
                {history.map((snapshot) => (
                  <div key={snapshot.id} className="snapshot-card">
                    <div>
                      <div className="snapshot-label">{snapshot.label}</div>
                      <div className="snapshot-time">{formatTimestamp(snapshot.createdAt)}</div>
                    </div>
                    <button type="button" className="toolbar-button" onClick={() => restoreSnapshot(snapshot.id)}>
                      Restore
                    </button>
                  </div>
                ))}
              </div>
            </div>
          </aside>
        </section>
      </main>

      <input
        ref={fileInputRef}
        type="file"
        className="hidden-input"
        accept={uploadIntent?.kind.includes('model') ? '.glb' : 'image/*,.png,.jpg,.jpeg,.webp,.gif'}
        onChange={handleUploadSelection}
      />
    </div>
  )
}

export default App
