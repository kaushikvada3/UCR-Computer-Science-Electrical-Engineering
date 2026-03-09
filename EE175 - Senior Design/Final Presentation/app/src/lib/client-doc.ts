import * as Y from 'yjs'
import type {
  DeckDocument,
  ImageElement,
  ModelElement,
  SlideBackground,
  SlideElement,
  TableStyle,
  TextElement,
  TextStyle,
  ThemeTokens,
} from '../../../shared/src/deck.js'
import { seedDeck } from '../../../shared/src/deck.js'

const ensureTextMap = (elementMap: Y.Map<unknown>, value: string): Y.Text => {
  let textDoc = elementMap.get('yText') as Y.Text | undefined
  if (!(textDoc instanceof Y.Text)) {
    textDoc = new Y.Text(value)
    elementMap.set('yText', textDoc)
  }
  return textDoc
}

const readTextElement = (map: Y.Map<unknown>): TextElement => {
  const yText = ensureTextMap(map, String(map.get('text') ?? ''))
  return {
    id: String(map.get('id')),
    type: 'text',
    name: String(map.get('name')),
    x: Number(map.get('x')),
    y: Number(map.get('y')),
    w: Number(map.get('w')),
    h: Number(map.get('h')),
    rotation: Number(map.get('rotation')),
    zIndex: Number(map.get('zIndex')),
    locked: Boolean(map.get('locked')),
    text: yText.toString(),
    style: map.get('style') as TextStyle,
  }
}

const readElement = (map: Y.Map<unknown>): SlideElement => {
  const type = String(map.get('type')) as SlideElement['type']
  if (type === 'text') {
    return readTextElement(map)
  }
  if (type === 'image') {
    return {
      id: String(map.get('id')),
      type: 'image',
      name: String(map.get('name')),
      x: Number(map.get('x')),
      y: Number(map.get('y')),
      w: Number(map.get('w')),
      h: Number(map.get('h')),
      rotation: Number(map.get('rotation')),
      zIndex: Number(map.get('zIndex')),
      locked: Boolean(map.get('locked')),
      src: String(map.get('src')),
      alt: String(map.get('alt')),
      fit: String(map.get('fit')) as ImageElement['fit'],
      caption: String(map.get('caption')),
      background: String(map.get('background')),
      borderColor: String(map.get('borderColor')),
      borderWidth: Number(map.get('borderWidth')),
      radius: Number(map.get('radius')),
      shadow: String(map.get('shadow')),
      preserveAspectRatio: Boolean(map.get('preserveAspectRatio')),
    }
  }
  if (type === 'model') {
    return {
      id: String(map.get('id')),
      type: 'model',
      name: String(map.get('name')),
      x: Number(map.get('x')),
      y: Number(map.get('y')),
      w: Number(map.get('w')),
      h: Number(map.get('h')),
      rotation: Number(map.get('rotation')),
      zIndex: Number(map.get('zIndex')),
      locked: Boolean(map.get('locked')),
      src: String(map.get('src')),
      poster: String(map.get('poster')),
      caption: String(map.get('caption')),
      fit: String(map.get('fit')) as ModelElement['fit'],
      autoRotate: Boolean(map.get('autoRotate')),
      cameraControls: Boolean(map.get('cameraControls')),
      background: String(map.get('background')),
      borderColor: String(map.get('borderColor')),
      borderWidth: Number(map.get('borderWidth')),
      radius: Number(map.get('radius')),
      shadow: String(map.get('shadow')),
      preserveAspectRatio: Boolean(map.get('preserveAspectRatio')),
    }
  }
  if (type === 'shape') {
    return {
      id: String(map.get('id')),
      type: 'shape',
      name: String(map.get('name')),
      x: Number(map.get('x')),
      y: Number(map.get('y')),
      w: Number(map.get('w')),
      h: Number(map.get('h')),
      rotation: Number(map.get('rotation')),
      zIndex: Number(map.get('zIndex')),
      locked: Boolean(map.get('locked')),
      fill: String(map.get('fill')),
      borderColor: String(map.get('borderColor')),
      borderWidth: Number(map.get('borderWidth')),
      radius: Number(map.get('radius')),
      shadow: String(map.get('shadow')),
      opacity: Number(map.get('opacity')),
    }
  }
  return {
    id: String(map.get('id')),
    type: 'table',
    name: String(map.get('name')),
    x: Number(map.get('x')),
    y: Number(map.get('y')),
    w: Number(map.get('w')),
    h: Number(map.get('h')),
    rotation: Number(map.get('rotation')),
    zIndex: Number(map.get('zIndex')),
    locked: Boolean(map.get('locked')),
    columns: (map.get('columns') as string[]) ?? [],
    rows: (map.get('rows') as string[][]) ?? [],
    style: map.get('style') as TableStyle,
  }
}

export const readDeckDoc = (doc: Y.Doc): DeckDocument => {
  const meta = doc.getMap('deck')
  const slidesArray = doc.getArray<Y.Map<unknown>>('slides')
  return {
    id: String(meta.get('id') ?? seedDeck.id),
    title: String(meta.get('title') ?? seedDeck.title),
    size: (meta.get('size') as DeckDocument['size']) ?? seedDeck.size,
    theme: (meta.get('theme') as ThemeTokens) ?? seedDeck.theme,
    slides: slidesArray.toArray().map((slideMap: Y.Map<unknown>) => {
      const elements = slideMap.get('elements') as Y.Array<Y.Map<unknown>>
      return {
        id: String(slideMap.get('id')),
        name: String(slideMap.get('name')),
        background: slideMap.get('background') as SlideBackground,
        elements: elements.toArray().map(readElement),
      }
    }),
  }
}

export const getElementMap = (doc: Y.Doc, slideId: string, elementId: string) => {
  const slidesArray = doc.getArray<Y.Map<unknown>>('slides')
  const slideMap = slidesArray.toArray().find((slide: Y.Map<unknown>) => slide.get('id') === slideId)
  if (!slideMap) {
    return null
  }
  const elements = slideMap.get('elements') as Y.Array<Y.Map<unknown>>
  return elements.toArray().find((element) => element.get('id') === elementId) ?? null
}

export const getSlideMap = (doc: Y.Doc, slideId: string) => {
  const slidesArray = doc.getArray<Y.Map<unknown>>('slides')
  return slidesArray.toArray().find((slide: Y.Map<unknown>) => slide.get('id') === slideId) ?? null
}
