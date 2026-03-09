import * as Y from 'yjs'
import type {
  DeckDocument,
  ImageElement,
  ModelElement,
  ShapeElement,
  SlideElement,
  TableElement,
  TextElement,
  TextStyle,
} from '../../../shared/src/deck.js'
import {
  cloneElement,
  seedDeck,
} from '../../../shared/src/deck.js'
import { getElementMap, getSlideMap } from './client-doc'

export const LOCAL_ORIGIN = Symbol('local-editor-origin')

export type BootstrapPayload = {
  deckId: string
  roomId: string
  shareToken: string
  theme: DeckDocument['theme']
  assetBaseUrl: string
  websocketPath: string
  collaboratorLabel: string
}

export type SnapshotMeta = {
  id: string
  label: string
  createdAt: string
  autosave: boolean
}

export type AwarenessUserState = {
  name: string
  color: string
  currentSlideId: string | null
  selectedElementIds: string[]
  editingTextId: string | null
}

export const resolveAssetUrl = (src: string, assetBaseUrl: string) => {
  if (!src) {
    return ''
  }
  if (src.startsWith('http://') || src.startsWith('https://') || src.startsWith('/')) {
    return src.startsWith('/') ? `${getApiBaseUrl()}${src}` : src
  }
  const base = assetBaseUrl.startsWith('http://') || assetBaseUrl.startsWith('https://')
    ? assetBaseUrl
    : `${getApiBaseUrl()}${assetBaseUrl}`
  return `${base.replace(/\/$/, '')}/${src}`
}

export const applyTextChange = (yText: Y.Text, nextValue: string) => {
  const current = yText.toString()
  if (current === nextValue) {
    return
  }
  let start = 0
  while (start < current.length && start < nextValue.length && current[start] === nextValue[start]) {
    start += 1
  }
  let currentEnd = current.length - 1
  let nextEnd = nextValue.length - 1
  while (currentEnd >= start && nextEnd >= start && current[currentEnd] === nextValue[nextEnd]) {
    currentEnd -= 1
    nextEnd -= 1
  }
  if (currentEnd >= start) {
    yText.delete(start, currentEnd - start + 1)
  }
  if (nextEnd >= start) {
    yText.insert(start, nextValue.slice(start, nextEnd + 1))
  }
}

export const updateLocalAwareness = (
  awareness: {
    setLocalStateField: (field: string, value: unknown) => void
  },
  state: AwarenessUserState,
) => {
  awareness.setLocalStateField('user', {
    name: state.name,
    color: state.color,
  })
  awareness.setLocalStateField('currentSlideId', state.currentSlideId)
  awareness.setLocalStateField('selectedElementIds', state.selectedElementIds)
  awareness.setLocalStateField('editingTextId', state.editingTextId)
}

export const reorderSlides = (doc: Y.Doc, activeId: string, overId: string) => {
  if (activeId === overId) {
    return
  }
  const slides = doc.getArray<Y.Map<unknown>>('slides')
  const slideMaps = slides.toArray()
  const oldIndex = slideMaps.findIndex((slide) => slide.get('id') === activeId)
  const newIndex = slideMaps.findIndex((slide) => slide.get('id') === overId)
  if (oldIndex < 0 || newIndex < 0) {
    return
  }
  const [moved] = slideMaps.splice(oldIndex, 1)
  slideMaps.splice(newIndex, 0, moved)
  doc.transact(() => {
    slides.delete(0, slides.length)
    slides.insert(0, slideMaps)
  }, LOCAL_ORIGIN)
}

export const updateElementProps = (doc: Y.Doc, slideId: string, elementId: string, updates: Record<string, unknown>) => {
  const elementMap = getElementMap(doc, slideId, elementId)
  if (!elementMap) {
    return
  }
  doc.transact(() => {
    Object.entries(updates).forEach(([key, value]) => {
      elementMap.set(key, value)
    })
  }, LOCAL_ORIGIN)
}

export const updateTextStyle = (doc: Y.Doc, slideId: string, elementId: string, updates: Partial<TextStyle>) => {
  const elementMap = getElementMap(doc, slideId, elementId)
  if (!elementMap || elementMap.get('type') !== 'text') {
    return
  }
  const currentStyle = elementMap.get('style') as TextStyle
  doc.transact(() => {
    elementMap.set('style', { ...currentStyle, ...updates })
  }, LOCAL_ORIGIN)
}

export const updateTextValue = (doc: Y.Doc, slideId: string, elementId: string, nextValue: string) => {
  const elementMap = getElementMap(doc, slideId, elementId)
  if (!elementMap || elementMap.get('type') !== 'text') {
    return
  }
  const yText = elementMap.get('yText') as Y.Text | undefined
  if (!yText) {
    return
  }
  doc.transact(() => {
    applyTextChange(yText, nextValue)
  }, LOCAL_ORIGIN)
}

export const removeElements = (doc: Y.Doc, slideId: string, elementIds: string[]) => {
  const slideMap = getSlideMap(doc, slideId)
  if (!slideMap) {
    return
  }
  const elements = slideMap.get('elements') as Y.Array<Y.Map<unknown>>
  const indexes = elements
    .toArray()
    .map((element, index) => ({ id: String(element.get('id')), index }))
    .filter((entry) => elementIds.includes(entry.id))
    .map((entry) => entry.index)
    .sort((left, right) => right - left)

  doc.transact(() => {
    indexes.forEach((index) => {
      elements.delete(index, 1)
    })
  }, LOCAL_ORIGIN)
}

const writeElementMap = (map: Y.Map<unknown>, element: SlideElement) => {
  Object.entries(element).forEach(([key, value]) => {
    if (key === 'text' && element.type === 'text') {
      const yText = new Y.Text()
      map.set('yText', yText)
      if (value) {
        yText.insert(0, value)
      }
      return
    }
    map.set(key, value as never)
  })
}

export const duplicateElements = (doc: Y.Doc, slideId: string, elementsToDuplicate: SlideElement[]) => {
  const slideMap = getSlideMap(doc, slideId)
  if (!slideMap) {
    return []
  }
  const elements = slideMap.get('elements') as Y.Array<Y.Map<unknown>>
  const insertedIds: string[] = []
  doc.transact(() => {
    elementsToDuplicate.forEach((element) => {
      const next = cloneElement(element, crypto.randomUUID())
      const map = new Y.Map<unknown>()
      elements.push([map])
      writeElementMap(map, next)
      insertedIds.push(next.id)
    })
  }, LOCAL_ORIGIN)
  return insertedIds
}

export const nudgeElements = (doc: Y.Doc, slideId: string, elementIds: string[], dx: number, dy: number) => {
  doc.transact(() => {
    elementIds.forEach((elementId) => {
      const elementMap = getElementMap(doc, slideId, elementId)
      if (!elementMap) {
        return
      }
      elementMap.set('x', Number(elementMap.get('x')) + dx)
      elementMap.set('y', Number(elementMap.get('y')) + dy)
    })
  }, LOCAL_ORIGIN)
}

const baseElement = (name: string): Omit<SlideElement, 'type'> & Record<string, unknown> => ({
  id: crypto.randomUUID(),
  name,
  x: 120,
  y: 180,
  w: 340,
  h: 140,
  rotation: 0,
  zIndex: 30,
  locked: false,
})

export const createTextElement = (): TextElement => ({
  ...(baseElement('Text Box') as Omit<TextElement, 'type' | 'text' | 'style'>),
  type: 'text',
  text: 'Double-click to edit',
  style: {
    fontFamily: seedDeck.theme.fontBody,
    fontSize: 22,
    fontWeight: 700,
    italic: false,
    color: seedDeck.theme.colors.textPrimary,
    align: 'left',
    lineHeight: 1.35,
    letterSpacing: 0,
    bullet: false,
    uppercase: false,
    background: 'rgba(255,255,255,0.06)',
    borderColor: 'rgba(255,255,255,0.14)',
    borderWidth: 1,
    radius: 24,
    padding: 16,
    shadow: 'none',
  },
})

export const createShapeElement = (): ShapeElement => ({
  ...(baseElement('Shape') as Omit<ShapeElement, 'type' | 'fill' | 'borderColor' | 'borderWidth' | 'radius' | 'shadow' | 'opacity'>),
  type: 'shape',
  w: 280,
  h: 180,
  fill: 'rgba(10,132,255,0.16)',
  borderColor: 'rgba(10,132,255,0.4)',
  borderWidth: 1,
  radius: 28,
  shadow: seedDeck.theme.shadow,
  opacity: 1,
})

export const createImageElement = (src: string, name = 'Image'): ImageElement => ({
  ...(baseElement(name) as Omit<ImageElement, 'type' | 'src' | 'alt' | 'fit' | 'caption' | 'background' | 'borderColor' | 'borderWidth' | 'radius' | 'shadow' | 'preserveAspectRatio'>),
  type: 'image',
  w: 360,
  h: 240,
  src,
  alt: name,
  fit: 'contain',
  caption: 'Editable caption',
  background: 'rgba(0,0,0,0.18)',
  borderColor: 'rgba(255,255,255,0.14)',
  borderWidth: 1,
  radius: 24,
  shadow: seedDeck.theme.shadow,
  preserveAspectRatio: true,
})

export const createModelElement = (src: string, poster = '', name = '3D Model'): ModelElement => ({
  ...(baseElement(name) as Omit<ModelElement, 'type' | 'src' | 'poster' | 'caption' | 'fit' | 'autoRotate' | 'cameraControls' | 'background' | 'borderColor' | 'borderWidth' | 'radius' | 'shadow' | 'preserveAspectRatio'>),
  type: 'model',
  w: 380,
  h: 280,
  src,
  poster,
  caption: 'Interactive model',
  fit: 'contain',
  autoRotate: true,
  cameraControls: true,
  background: 'rgba(255,255,255,0.04)',
  borderColor: 'rgba(255,255,255,0.14)',
  borderWidth: 1,
  radius: 24,
  shadow: seedDeck.theme.shadow,
  preserveAspectRatio: true,
})

export const createTableElement = (): TableElement => ({
  ...(baseElement('Table') as Omit<TableElement, 'type' | 'columns' | 'rows' | 'style'>),
  type: 'table',
  w: 560,
  h: 320,
  columns: ['Column A', 'Column B', 'Column C'],
  rows: [
    ['Value 1', 'Value 2', 'Value 3'],
    ['Value 4', 'Value 5', 'Value 6'],
  ],
  style: {
    headerBackground: 'rgba(10,132,255,0.18)',
    headerColor: seedDeck.theme.colors.textPrimary,
    cellBackground: 'rgba(255,255,255,0.06)',
    cellColor: seedDeck.theme.colors.textSecondary,
    borderColor: 'rgba(255,255,255,0.14)',
    accentColor: seedDeck.theme.colors.accent,
    fontSize: 16,
  },
})

export const insertElement = (doc: Y.Doc, slideId: string, element: SlideElement) => {
  const slideMap = getSlideMap(doc, slideId)
  if (!slideMap) {
    return
  }
  const elements = slideMap.get('elements') as Y.Array<Y.Map<unknown>>
  const map = new Y.Map<unknown>()
  doc.transact(() => {
    elements.push([map])
    writeElementMap(map, element)
  }, LOCAL_ORIGIN)
}

export const updateTable = (doc: Y.Doc, slideId: string, elementId: string, updates: Partial<TableElement>) => {
  const elementMap = getElementMap(doc, slideId, elementId)
  if (!elementMap || elementMap.get('type') !== 'table') {
    return
  }
  doc.transact(() => {
    Object.entries(updates).forEach(([key, value]) => {
      elementMap.set(key, value)
    })
  }, LOCAL_ORIGIN)
}

export const getApiBaseUrl = () => {
  const protocol = window.location.protocol
  const host = window.location.hostname
  const port = window.location.port === '5173' ? '8787' : (window.location.port || (protocol === 'https:' ? '443' : '80'))
  return `${protocol}//${host}${port ? `:${port}` : ''}`
}

export const getSocketBaseUrl = () => {
  const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:'
  const host = window.location.hostname
  const port = window.location.port === '5173' ? '8787' : (window.location.port || (protocol === 'wss:' ? '443' : '80'))
  return `${protocol}//${host}${port ? `:${port}` : ''}`
}
