import * as ServerY from 'yjs'
import type {
  DeckDocument,
  ImageElement,
  ModelElement,
  ShapeElement,
  Slide,
  SlideBackground,
  SlideElement,
  TableElement,
  TableStyle,
  TextElement,
  TextStyle,
  ThemeTokens,
} from '../../shared/src/deck.js'
import { seedDeck } from '../../shared/src/deck.js'

const ensureYText = (elementMap: InstanceType<typeof ServerY.Map>, value: string): InstanceType<typeof ServerY.Text> => {
  let yText = elementMap.get('yText') as InstanceType<typeof ServerY.Text> | undefined
  if (!(yText instanceof ServerY.Text)) {
    yText = new ServerY.Text(value)
    elementMap.set('yText', yText)
  }
  return yText
}

const writeElementMap = (map: InstanceType<typeof ServerY.Map>, element: SlideElement) => {
  Object.entries(element).forEach(([key, value]) => {
    if (key === 'text' && element.type === 'text') {
      const yText = new ServerY.Text()
      map.set('yText', yText)
      if (value) {
        yText.insert(0, value)
      }
      return
    }
    map.set(key, value)
  })
}

export const initializeServerDeckDoc = (doc: any, deck: DeckDocument = seedDeck) => {
  const meta = doc.getMap('deck')
  const slidesArray = doc.getArray('slides')
  if (slidesArray.length > 0) {
    return
  }
  doc.transact(() => {
    meta.set('id', deck.id)
    meta.set('title', deck.title)
    meta.set('size', deck.size)
    meta.set('theme', deck.theme)
    deck.slides.forEach((slide: Slide) => {
      const slideMap = new ServerY.Map()
      const elements = new ServerY.Array()
      slideMap.set('elements', elements)
      slidesArray.push([slideMap])
      slideMap.set('id', slide.id)
      slideMap.set('name', slide.name)
      slideMap.set('background', slide.background)
      slide.elements.forEach((element: SlideElement) => {
        const elementMap = new ServerY.Map()
        elements.push([elementMap])
        writeElementMap(elementMap, element)
      })
    })
  }, 'seed')
}

const readTextElement = (map: any): TextElement => {
  const yText = ensureYText(map, String(map.get('text') ?? ''))
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

const readElement = (map: any): SlideElement => {
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

export const readServerDeckDoc = (doc: any): DeckDocument => {
  const meta = doc.getMap('deck')
  const slidesArray = doc.getArray('slides')
  return {
    id: String(meta.get('id') ?? seedDeck.id),
    title: String(meta.get('title') ?? seedDeck.title),
    size: (meta.get('size') as DeckDocument['size']) ?? seedDeck.size,
    theme: (meta.get('theme') as ThemeTokens) ?? seedDeck.theme,
    slides: slidesArray.toArray().map((slideMap: any) => {
      const elements = slideMap.get('elements')
      return {
        id: String(slideMap.get('id')),
        name: String(slideMap.get('name')),
        background: slideMap.get('background') as SlideBackground,
        elements: elements.toArray().map((element: any) => readElement(element)),
      }
    }),
  }
}

export const replaceServerDeckDoc = (doc: any, deck: DeckDocument) => {
  const meta = doc.getMap('deck')
  const slidesArray = doc.getArray('slides')
  doc.transact(() => {
    meta.clear()
    slidesArray.delete(0, slidesArray.length)
    meta.set('id', deck.id)
    meta.set('title', deck.title)
    meta.set('size', deck.size)
    meta.set('theme', deck.theme)
    deck.slides.forEach((slide: Slide) => {
      const slideMap = new ServerY.Map()
      const elements = new ServerY.Array()
      slideMap.set('elements', elements)
      slidesArray.push([slideMap])
      slideMap.set('id', slide.id)
      slideMap.set('name', slide.name)
      slideMap.set('background', slide.background)
      slide.elements.forEach((element: SlideElement) => {
        const elementMap = new ServerY.Map()
        elements.push([elementMap])
        writeElementMap(elementMap, element)
      })
    })
  }, 'replace-deck')
}

export const ServerYjs = ServerY
