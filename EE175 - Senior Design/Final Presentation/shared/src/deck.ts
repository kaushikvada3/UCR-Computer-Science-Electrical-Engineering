export const DECK_ID = 'bms-final-design'
export const SHARE_TOKEN = 'bms-final-editor'
export const ASSET_BASE_PATH = '/assets'
export const SLIDE_WIDTH = 1600
export const SLIDE_HEIGHT = 900

export type ThemeTokens = {
  fontBody: string
  fontDisplay: string
  colors: {
    background: string
    backgroundAlt: string
    panel: string
    panelStrong: string
    border: string
    textPrimary: string
    textSecondary: string
    textMuted: string
    accent: string
    accentSoft: string
    success: string
    warning: string
    danger: string
  }
  shadow: string
}

export type SlideBackground = {
  start: string
  mid: string
  end: string
}

export type BaseElement = {
  id: string
  type: 'text' | 'image' | 'model' | 'shape' | 'table'
  x: number
  y: number
  w: number
  h: number
  rotation: number
  zIndex: number
  locked: boolean
  name: string
}

export type TextStyle = {
  fontFamily: string
  fontSize: number
  fontWeight: number
  italic: boolean
  color: string
  align: 'left' | 'center' | 'right'
  lineHeight: number
  letterSpacing: number
  bullet: boolean
  uppercase: boolean
  background: string
  borderColor: string
  borderWidth: number
  radius: number
  padding: number
  shadow: string
}

export type TextElement = BaseElement & {
  type: 'text'
  text: string
  style: TextStyle
}

export type ImageElement = BaseElement & {
  type: 'image'
  src: string
  alt: string
  fit: 'contain' | 'cover'
  caption: string
  background: string
  borderColor: string
  borderWidth: number
  radius: number
  shadow: string
  preserveAspectRatio: boolean
}

export type ModelElement = BaseElement & {
  type: 'model'
  src: string
  poster: string
  caption: string
  fit: 'contain' | 'cover'
  autoRotate: boolean
  cameraControls: boolean
  background: string
  borderColor: string
  borderWidth: number
  radius: number
  shadow: string
  preserveAspectRatio: boolean
}

export type ShapeElement = BaseElement & {
  type: 'shape'
  fill: string
  borderColor: string
  borderWidth: number
  radius: number
  shadow: string
  opacity: number
}

export type TableStyle = {
  headerBackground: string
  headerColor: string
  cellBackground: string
  cellColor: string
  borderColor: string
  accentColor: string
  fontSize: number
}

export type TableElement = BaseElement & {
  type: 'table'
  columns: string[]
  rows: string[][]
  style: TableStyle
}

export type SlideElement =
  | TextElement
  | ImageElement
  | ModelElement
  | ShapeElement
  | TableElement

export type Slide = {
  id: string
  name: string
  background: SlideBackground
  elements: SlideElement[]
}

export type DeckDocument = {
  id: string
  title: string
  size: {
    width: number
    height: number
  }
  theme: ThemeTokens
  slides: Slide[]
}

const theme: ThemeTokens = {
  fontBody: '"Inter", sans-serif',
  fontDisplay: '"Outfit", sans-serif',
  colors: {
    background: '#0d1017',
    backgroundAlt: '#12080a',
    panel: 'rgba(255, 255, 255, 0.06)',
    panelStrong: 'rgba(255, 255, 255, 0.1)',
    border: 'rgba(255, 255, 255, 0.14)',
    textPrimary: '#f7f8fb',
    textSecondary: 'rgba(255, 255, 255, 0.82)',
    textMuted: 'rgba(255, 255, 255, 0.56)',
    accent: '#0a84ff',
    accentSoft: 'rgba(10, 132, 255, 0.18)',
    success: '#30d158',
    warning: '#ffd60a',
    danger: '#ff453a',
  },
  shadow: '0 28px 70px rgba(0, 0, 0, 0.42)',
}

const defaultBackground: SlideBackground = {
  start: 'rgba(114, 23, 14, 0.96)',
  mid: 'rgba(92, 12, 11, 0.92)',
  end: 'rgba(8, 11, 19, 0.96)',
}

const id = (...parts: string[]) => parts.join('-')

const baseTextStyle = (overrides: Partial<TextStyle> = {}): TextStyle => ({
  fontFamily: theme.fontBody,
  fontSize: 18,
  fontWeight: 500,
  italic: false,
  color: theme.colors.textSecondary,
  align: 'left',
  lineHeight: 1.45,
  letterSpacing: 0,
  bullet: false,
  uppercase: false,
  background: 'transparent',
  borderColor: 'transparent',
  borderWidth: 0,
  radius: 24,
  padding: 0,
  shadow: 'none',
  ...overrides,
})

const text = (element: Omit<TextElement, 'type' | 'rotation' | 'locked'> & Partial<Pick<TextElement, 'rotation' | 'locked'>>): TextElement => ({
  type: 'text',
  rotation: 0,
  locked: false,
  ...element,
})

const image = (element: Omit<ImageElement, 'type' | 'rotation' | 'locked'> & Partial<Pick<ImageElement, 'rotation' | 'locked'>>): ImageElement => ({
  type: 'image',
  rotation: 0,
  locked: false,
  ...element,
})

const model = (element: Omit<ModelElement, 'type' | 'rotation' | 'locked'> & Partial<Pick<ModelElement, 'rotation' | 'locked'>>): ModelElement => ({
  type: 'model',
  rotation: 0,
  locked: false,
  ...element,
})

const shape = (element: Omit<ShapeElement, 'type' | 'rotation' | 'locked'> & Partial<Pick<ShapeElement, 'rotation' | 'locked'>>): ShapeElement => ({
  type: 'shape',
  rotation: 0,
  locked: false,
  ...element,
})

const table = (element: Omit<TableElement, 'type' | 'rotation' | 'locked'> & Partial<Pick<TableElement, 'rotation' | 'locked'>>): TableElement => ({
  type: 'table',
  rotation: 0,
  locked: false,
  ...element,
})

const glassFill = 'linear-gradient(180deg, rgba(255, 255, 255, 0.1) 0%, rgba(255, 255, 255, 0.05) 100%)'
const cardFill = 'rgba(255, 255, 255, 0.06)'
const thinBorder = theme.colors.border

const chrome = (slideNo: number, section: string, tab: string): SlideElement[] => [
  text({
    id: id('slide', String(slideNo), 'mark'),
    name: 'Slide Mark',
    x: 56,
    y: 40,
    w: 132,
    h: 34,
    zIndex: 10,
    text: `Slide ${slideNo}`,
    style: baseTextStyle({
      fontFamily: theme.fontBody,
      fontSize: 12,
      fontWeight: 800,
      color: theme.colors.textPrimary,
      uppercase: true,
      letterSpacing: 2.6,
      align: 'center',
      background: cardFill,
      borderColor: thinBorder,
      borderWidth: 1,
      radius: 999,
      padding: 10,
    }),
  }),
  text({
    id: id('slide', String(slideNo), 'section'),
    name: 'Section Label',
    x: 208,
    y: 42,
    w: 300,
    h: 18,
    zIndex: 10,
    text: section.toUpperCase(),
    style: baseTextStyle({
      fontSize: 11,
      fontWeight: 800,
      color: theme.colors.textMuted,
      uppercase: true,
      letterSpacing: 3.2,
    }),
  }),
  text({
    id: id('slide', String(slideNo), 'subsection'),
    name: 'Section Subtitle',
    x: 208,
    y: 60,
    w: 360,
    h: 18,
    zIndex: 10,
    text: section,
    style: baseTextStyle({
      fontSize: 13,
      fontWeight: 600,
      color: theme.colors.textSecondary,
    }),
  }),
  text({
    id: id('slide', String(slideNo), 'tab'),
    name: 'Tab Badge',
    x: 1326,
    y: 40,
    w: 200,
    h: 34,
    zIndex: 10,
    text: tab,
    style: baseTextStyle({
      fontSize: 14,
      fontWeight: 700,
      color: theme.colors.textPrimary,
      align: 'center',
      background: 'rgba(255,255,255,0.08)',
      borderColor: thinBorder,
      borderWidth: 1,
      radius: 999,
      padding: 10,
    }),
  }),
  shape({
    id: id('slide', String(slideNo), 'divider'),
    name: 'Top Divider',
    x: 54,
    y: 96,
    w: 1490,
    h: 1,
    zIndex: 2,
    fill: 'rgba(255, 255, 255, 0.14)',
    borderColor: 'transparent',
    borderWidth: 0,
    radius: 1,
    shadow: 'none',
    opacity: 1,
  }),
]

const panel = (slideNo: number, panelId: string, x: number, y: number, w: number, h: number, titleText: string, bodyText: string, options: Partial<TextStyle> = {}): SlideElement[] => [
  shape({
    id: id('slide', String(slideNo), panelId, 'bg'),
    name: `${titleText} Panel`,
    x,
    y,
    w,
    h,
    zIndex: 2,
    fill: cardFill,
    borderColor: thinBorder,
    borderWidth: 1,
    radius: 24,
    shadow: 'inset 0 1px 0 rgba(255,255,255,0.12)',
    opacity: 1,
  }),
  text({
    id: id('slide', String(slideNo), panelId, 'title'),
    name: `${titleText} Title`,
    x: x + 18,
    y: y + 16,
    w: w - 36,
    h: 34,
    zIndex: 3,
    text: titleText,
    style: baseTextStyle({
      fontFamily: theme.fontDisplay,
      fontSize: 22,
      fontWeight: 700,
      color: theme.colors.textPrimary,
    }),
  }),
  text({
    id: id('slide', String(slideNo), panelId, 'body'),
    name: `${titleText} Body`,
    x: x + 18,
    y: y + 58,
    w: w - 36,
    h: h - 72,
    zIndex: 3,
    text: bodyText,
    style: baseTextStyle({
      fontSize: 16,
      fontWeight: 500,
      color: theme.colors.textSecondary,
      bullet: true,
      lineHeight: 1.5,
      ...options,
    }),
  }),
]

const seededSlides: Slide[] = [
  {
    id: 'slide-1',
    name: 'Title',
    background: defaultBackground,
    elements: [
      ...chrome(1, 'Final Presentation', 'E-Load'),
      text({
        id: 'slide-1-title',
        name: 'Hero Title',
        x: 70,
        y: 148,
        w: 720,
        h: 160,
        zIndex: 4,
        text: 'Battery Management\nand Communication System',
        style: baseTextStyle({
          fontFamily: theme.fontDisplay,
          fontSize: 72,
          fontWeight: 900,
          color: theme.colors.textPrimary,
          lineHeight: 0.98,
          letterSpacing: -2.6,
        }),
      }),
      text({
        id: 'slide-1-lead',
        name: 'Hero Lead',
        x: 76,
        y: 346,
        w: 640,
        h: 220,
        zIndex: 4,
        text: '10s1p Li-ion instrumentation and control\nBQ76930 monitoring and STM32 firmware\nOperator dashboard for telemetry and fan control\nJetson-ready path for battery analytics',
        style: baseTextStyle({
          fontSize: 22,
          fontWeight: 500,
          bullet: true,
          lineHeight: 1.35,
          color: 'rgba(255,255,255,0.88)',
        }),
      }),
      ...panel(1, 'meta-team', 74, 618, 220, 114, 'Team', 'Kaushik Vada\nJoshua Jensen\nHector Valladares', {
        bullet: true,
        fontSize: 15,
      }),
      ...panel(1, 'meta-stack', 312, 618, 260, 114, 'System', 'BQ76930 monitor\nSTM32F303RC control\nDesktop dashboard', {
        bullet: true,
        fontSize: 15,
      }),
      ...panel(1, 'meta-status', 590, 618, 230, 114, 'Status', 'Integrated hardware\nBench telemetry ready\nAI expansion path', {
        bullet: true,
        fontSize: 15,
      }),
      shape({
        id: 'slide-1-hero-shell',
        name: 'Hero Model Shell',
        x: 900,
        y: 148,
        w: 610,
        h: 608,
        zIndex: 1,
        fill: glassFill,
        borderColor: thinBorder,
        borderWidth: 1,
        radius: 34,
        shadow: theme.shadow,
        opacity: 1,
      }),
      model({
        id: 'slide-1-model',
        name: 'BMS Model',
        x: 958,
        y: 182,
        w: 500,
        h: 470,
        zIndex: 4,
        src: 'BMS.glb',
        poster: 'BMS_Image.png',
        caption: 'Interactive enclosure render',
        fit: 'contain',
        autoRotate: true,
        cameraControls: true,
        background: 'transparent',
        borderColor: 'transparent',
        borderWidth: 0,
        radius: 22,
        shadow: 'none',
        preserveAspectRatio: true,
      }),
      text({
        id: 'slide-1-callout',
        name: 'Hero Callout',
        x: 946,
        y: 680,
        w: 216,
        h: 64,
        zIndex: 4,
        text: '10 cells\n10 NTCs',
        style: baseTextStyle({
          fontSize: 17,
          fontWeight: 700,
          align: 'center',
          color: theme.colors.textPrimary,
          background: cardFill,
          borderColor: thinBorder,
          borderWidth: 1,
          radius: 22,
          padding: 12,
        }),
      }),
      text({
        id: 'slide-1-callout-2',
        name: 'Telemetry Callout',
        x: 1180,
        y: 680,
        w: 270,
        h: 64,
        zIndex: 4,
        text: 'USB telemetry\nOperator dashboard',
        style: baseTextStyle({
          fontSize: 17,
          fontWeight: 700,
          align: 'center',
          color: theme.colors.textPrimary,
          background: cardFill,
          borderColor: thinBorder,
          borderWidth: 1,
          radius: 22,
          padding: 12,
        }),
      }),
    ],
  },
  {
    id: 'slide-2',
    name: 'Concept',
    background: defaultBackground,
    elements: [
      ...chrome(2, 'Concept and Application', 'Applications'),
      text({
        id: 'slide-2-title',
        name: 'Slide Title',
        x: 68,
        y: 128,
        w: 720,
        h: 60,
        zIndex: 4,
        text: 'Concept and Application of the Design',
        style: baseTextStyle({
          fontFamily: theme.fontDisplay,
          fontSize: 46,
          fontWeight: 800,
          color: theme.colors.textPrimary,
          letterSpacing: -1.5,
        }),
      }),
      text({
        id: 'slide-2-subhead',
        name: 'Slide Subhead',
        x: 70,
        y: 196,
        w: 920,
        h: 90,
        zIndex: 4,
        text: 'Split-domain architecture for safety\nPower domain handles battery sensing, cooling, and protection\nHost domain runs STM32 control and the operator interface',
        style: baseTextStyle({
          fontSize: 20,
          bullet: true,
          lineHeight: 1.35,
        }),
      }),
      shape({
        id: 'slide-2-model-a-bg',
        name: 'BMS Model Panel',
        x: 68,
        y: 310,
        w: 470,
        h: 500,
        zIndex: 1,
        fill: cardFill,
        borderColor: thinBorder,
        borderWidth: 1,
        radius: 28,
        shadow: theme.shadow,
        opacity: 1,
      }),
      model({
        id: 'slide-2-model-a',
        name: 'BMS Model',
        x: 96,
        y: 338,
        w: 414,
        h: 360,
        zIndex: 4,
        src: 'BMS.glb',
        poster: 'BMS_Image.png',
        caption: 'Final BMS enclosure render',
        fit: 'contain',
        autoRotate: true,
        cameraControls: true,
        background: 'transparent',
        borderColor: 'transparent',
        borderWidth: 0,
        radius: 18,
        shadow: 'none',
        preserveAspectRatio: true,
      }),
      text({
        id: 'slide-2-model-a-caption',
        name: 'BMS Model Caption',
        x: 100,
        y: 720,
        w: 390,
        h: 40,
        zIndex: 4,
        text: 'Power-domain packaging, monitoring, and cooling hardware',
        style: baseTextStyle({
          fontSize: 15,
          color: theme.colors.textSecondary,
          align: 'center',
        }),
      }),
      shape({
        id: 'slide-2-model-b-bg',
        name: 'Load Model Panel',
        x: 560,
        y: 310,
        w: 470,
        h: 500,
        zIndex: 1,
        fill: cardFill,
        borderColor: thinBorder,
        borderWidth: 1,
        radius: 28,
        shadow: theme.shadow,
        opacity: 1,
      }),
      model({
        id: 'slide-2-model-b',
        name: 'E-Load Model',
        x: 588,
        y: 338,
        w: 414,
        h: 360,
        zIndex: 4,
        src: 'E-Load.glb',
        poster: 'E-Load Image.png',
        caption: 'Battery pack and enclosure test hardware',
        fit: 'contain',
        autoRotate: true,
        cameraControls: true,
        background: 'transparent',
        borderColor: 'transparent',
        borderWidth: 0,
        radius: 18,
        shadow: 'none',
        preserveAspectRatio: true,
      }),
      text({
        id: 'slide-2-model-b-caption',
        name: 'Load Model Caption',
        x: 592,
        y: 720,
        w: 390,
        h: 40,
        zIndex: 4,
        text: 'Interactive E-Load reference used for test integration context',
        style: baseTextStyle({
          fontSize: 15,
          color: theme.colors.textSecondary,
          align: 'center',
        }),
      }),
      ...panel(2, 'applications', 1050, 310, 458, 154, 'Applications', 'Lab battery characterization\nEmbedded controls experiments\nAI battery health estimation on Jetson'),
      ...panel(2, 'skills', 1050, 484, 458, 154, 'Skills Demonstrated', 'Power electronics and embedded systems\nPCB design and controls\nHardware-software co-design'),
      ...panel(2, 'advantages', 1050, 658, 458, 154, 'Why This Partitioning Works', 'Improves observability\nReduces fault propagation risk\nEnables independent bring-up and testing'),
    ],
  },
  {
    id: 'slide-3',
    name: 'Objectives',
    background: defaultBackground,
    elements: [
      ...chrome(3, 'Technical Objectives', 'Metrics'),
      text({
        id: 'slide-3-title',
        name: 'Slide Title',
        x: 68,
        y: 128,
        w: 760,
        h: 60,
        zIndex: 4,
        text: 'Technical Design Objectives',
        style: baseTextStyle({
          fontFamily: theme.fontDisplay,
          fontSize: 48,
          fontWeight: 800,
          color: theme.colors.textPrimary,
          letterSpacing: -1.8,
        }),
      }),
      text({
        id: 'slide-3-subhead',
        name: 'Slide Subhead',
        x: 72,
        y: 194,
        w: 1000,
        h: 34,
        zIndex: 4,
        text: 'Translating project requirements into explicit numerical targets from the design report and firmware code.',
        style: baseTextStyle({
          fontSize: 20,
          color: 'rgba(255,255,255,0.86)',
        }),
      }),
      ...panel(3, 'metric-a', 70, 270, 335, 190, 'Voltage Visibility', 'Monitor all 10 series cells\nTrack pack current through 20mΩ shunt\nMeasure 10 thermistors for thermal state', {
        fontSize: 17,
      }),
      ...panel(3, 'metric-b', 430, 270, 335, 190, 'Control Loop', '1 kHz PWM fan drive\n2°C hysteresis on thermal control\n2s host command timeout fallback', {
        fontSize: 17,
      }),
      ...panel(3, 'metric-c', 790, 270, 335, 190, 'Communications', 'USB CDC telemetry\n115200 baud framing\nParser resynchronization support', {
        fontSize: 17,
      }),
      ...panel(3, 'metric-d', 1150, 270, 335, 190, 'Expansion Path', 'Jetson analytics hook\nState estimation ready data stream\nUI command surface for testing', {
        fontSize: 17,
      }),
      shape({
        id: 'slide-3-goals-bg',
        name: 'Goals Panel',
        x: 70,
        y: 496,
        w: 1415,
        h: 286,
        zIndex: 1,
        fill: glassFill,
        borderColor: thinBorder,
        borderWidth: 1,
        radius: 30,
        shadow: theme.shadow,
        opacity: 1,
      }),
      text({
        id: 'slide-3-goals-title',
        name: 'Goals Title',
        x: 100,
        y: 528,
        w: 500,
        h: 44,
        zIndex: 4,
        text: 'Success Means',
        style: baseTextStyle({
          fontFamily: theme.fontDisplay,
          fontSize: 30,
          fontWeight: 800,
          color: theme.colors.textPrimary,
        }),
      }),
      text({
        id: 'slide-3-goals-body',
        name: 'Goals Body',
        x: 102,
        y: 586,
        w: 1260,
        h: 160,
        zIndex: 4,
        text: 'A stable battery measurement stack, reliable host-device serial communication, closed-loop fan control, and a polished operator dashboard all working together as one system.\nThe editor version of this deck should preserve these objectives as directly editable slide content.',
        style: baseTextStyle({
          fontSize: 22,
          lineHeight: 1.45,
          color: 'rgba(255,255,255,0.88)',
        }),
      }),
      text({
        id: 'slide-3-note',
        name: 'Objectives Note',
        x: 1210,
        y: 532,
        w: 230,
        h: 180,
        zIndex: 4,
        text: 'Targets are editable\nValues are not baked into HTML\nShared document state becomes the source of truth',
        style: baseTextStyle({
          fontSize: 18,
          fontWeight: 700,
          bullet: true,
          color: theme.colors.textPrimary,
          background: cardFill,
          borderColor: thinBorder,
          borderWidth: 1,
          radius: 22,
          padding: 16,
        }),
      }),
    ],
  },
  {
    id: 'slide-4',
    name: 'High-Level Design',
    background: defaultBackground,
    elements: [
      ...chrome(4, 'System Architecture', 'SBD'),
      text({
        id: 'slide-4-title',
        name: 'Slide Title',
        x: 68,
        y: 128,
        w: 760,
        h: 60,
        zIndex: 4,
        text: 'Final High-Level Design',
        style: baseTextStyle({
          fontFamily: theme.fontDisplay,
          fontSize: 48,
          fontWeight: 800,
          color: theme.colors.textPrimary,
        }),
      }),
      text({
        id: 'slide-4-subhead',
        name: 'Slide Subhead',
        x: 72,
        y: 194,
        w: 900,
        h: 74,
        zIndex: 4,
        text: 'System partitioned into battery unit, control electronics, and operator station\nThe architecture isolates high-energy pack handling from the USB host interface',
        style: baseTextStyle({
          fontSize: 20,
          bullet: true,
          lineHeight: 1.35,
        }),
      }),
      shape({
        id: 'slide-4-image-bg',
        name: 'Block Diagram Frame',
        x: 68,
        y: 286,
        w: 980,
        h: 520,
        zIndex: 1,
        fill: glassFill,
        borderColor: thinBorder,
        borderWidth: 1,
        radius: 28,
        shadow: theme.shadow,
        opacity: 1,
      }),
      image({
        id: 'slide-4-diagram',
        name: 'System Block Diagram',
        x: 92,
        y: 314,
        w: 934,
        h: 448,
        zIndex: 4,
        src: 'system_block_diagram.png',
        alt: 'System block diagram',
        fit: 'contain',
        caption: 'Power, control, and operator domains',
        background: 'rgba(0,0,0,0.16)',
        borderColor: 'transparent',
        borderWidth: 0,
        radius: 18,
        shadow: 'none',
        preserveAspectRatio: true,
      }),
      ...panel(4, 'domains', 1070, 286, 420, 220, 'Domain Boundaries', 'Battery domain: cells, shunt, thermistors, fan\nControl domain: STM32 and USB bridge\nOperator domain: desktop dashboard and Jetson analytics'),
      ...panel(4, 'benefits', 1070, 526, 420, 126, 'Why It Matters', 'Safer bring-up\nCleaner debugging\nClear ownership between subsystems'),
      ...panel(4, 'editable', 1070, 670, 420, 136, 'Editor Benefit', 'Architecture diagrams can be replaced or resized directly in the browser\nNotes remain editable alongside visuals'),
    ],
  },
  {
    id: 'slide-5',
    name: 'Electrical Implementation',
    background: defaultBackground,
    elements: [
      ...chrome(5, 'Low-Level Design', 'Hardware'),
      text({
        id: 'slide-5-title',
        name: 'Slide Title',
        x: 68,
        y: 128,
        w: 940,
        h: 60,
        zIndex: 4,
        text: 'Final Low-Level Design: Electrical Implementation',
        style: baseTextStyle({
          fontFamily: theme.fontDisplay,
          fontSize: 46,
          fontWeight: 800,
          color: theme.colors.textPrimary,
        }),
      }),
      text({
        id: 'slide-5-subhead',
        name: 'Slide Subhead',
        x: 72,
        y: 194,
        w: 860,
        h: 36,
        zIndex: 4,
        text: 'Core low-level implementation across three main schematic pages.',
        style: baseTextStyle({
          fontSize: 20,
          color: 'rgba(255,255,255,0.86)',
        }),
      }),
      shape({
        id: 'slide-5-card-a-bg',
        name: 'BQ76930 Frame',
        x: 66,
        y: 274,
        w: 474,
        h: 508,
        zIndex: 1,
        fill: cardFill,
        borderColor: thinBorder,
        borderWidth: 1,
        radius: 28,
        shadow: theme.shadow,
        opacity: 1,
      }),
      shape({
        id: 'slide-5-card-b-bg',
        name: 'STM32 Frame',
        x: 562,
        y: 274,
        w: 474,
        h: 508,
        zIndex: 1,
        fill: cardFill,
        borderColor: thinBorder,
        borderWidth: 1,
        radius: 28,
        shadow: theme.shadow,
        opacity: 1,
      }),
      shape({
        id: 'slide-5-card-c-bg',
        name: 'Fan Frame',
        x: 1058,
        y: 274,
        w: 474,
        h: 508,
        zIndex: 1,
        fill: cardFill,
        borderColor: thinBorder,
        borderWidth: 1,
        radius: 28,
        shadow: theme.shadow,
        opacity: 1,
      }),
      image({
        id: 'slide-5-bq',
        name: 'BQ76930 Schematic',
        x: 86,
        y: 298,
        w: 434,
        h: 322,
        zIndex: 4,
        src: 'bms_schematic_bq76930.png',
        alt: 'BQ76930 schematic',
        fit: 'contain',
        caption: 'Cell supervision and measurement front-end',
        background: 'rgba(0,0,0,0.16)',
        borderColor: 'transparent',
        borderWidth: 0,
        radius: 18,
        shadow: 'none',
        preserveAspectRatio: true,
      }),
      image({
        id: 'slide-5-stm',
        name: 'STM32 Schematic',
        x: 582,
        y: 298,
        w: 434,
        h: 322,
        zIndex: 4,
        src: 'bms_schematic_stm32.png',
        alt: 'STM32 schematic',
        fit: 'contain',
        caption: 'Microcontroller, USB CDC, and sensing interfaces',
        background: 'rgba(0,0,0,0.16)',
        borderColor: 'transparent',
        borderWidth: 0,
        radius: 18,
        shadow: 'none',
        preserveAspectRatio: true,
      }),
      image({
        id: 'slide-5-fan',
        name: 'Fan Schematic',
        x: 1078,
        y: 298,
        w: 434,
        h: 322,
        zIndex: 4,
        src: 'bms_schematic_fan.png',
        alt: 'Fan control schematic',
        fit: 'contain',
        caption: 'PWM-controlled cooling path',
        background: 'rgba(0,0,0,0.16)',
        borderColor: 'transparent',
        borderWidth: 0,
        radius: 18,
        shadow: 'none',
        preserveAspectRatio: true,
      }),
      ...panel(5, 'bq-notes', 86, 640, 434, 122, 'BQ76930 Focus', 'Voltage taps, balancing support, and thermal inputs'),
      ...panel(5, 'stm-notes', 582, 640, 434, 122, 'STM32 Focus', 'Sampling, command handling, and serial telemetry'),
      ...panel(5, 'fan-notes', 1078, 640, 434, 122, 'Cooling Focus', '1 kHz PWM output with staged thresholds and hysteresis'),
    ],
  },
  {
    id: 'slide-6',
    name: 'Program Flow',
    background: defaultBackground,
    elements: [
      ...chrome(6, 'Low-Level Design', 'GUI Flow'),
      text({
        id: 'slide-6-title',
        name: 'Slide Title',
        x: 68,
        y: 128,
        w: 880,
        h: 60,
        zIndex: 4,
        text: 'Final Low-Level Design: Program Flow-Charts',
        style: baseTextStyle({
          fontFamily: theme.fontDisplay,
          fontSize: 46,
          fontWeight: 800,
          color: theme.colors.textPrimary,
        }),
      }),
      text({
        id: 'slide-6-subhead',
        name: 'Slide Subhead',
        x: 72,
        y: 194,
        w: 960,
        h: 36,
        zIndex: 4,
        text: 'Implementation flow logic synthesized directly from firmware and dashboard operations.',
        style: baseTextStyle({
          fontSize: 20,
          color: 'rgba(255,255,255,0.86)',
        }),
      }),
      ...panel(6, 'firmware-flow', 70, 280, 700, 470, 'Firmware Runtime Flow', 'Boot peripherals and measurement stack\nRead cells, current, and temperatures\nEvaluate fan thresholds and balancing windows\nStream telemetry to USB CDC\nFail safe on stale host commands', {
        fontSize: 21,
      }),
      ...panel(6, 'dashboard-flow', 820, 280, 700, 470, 'Desktop Dashboard Flow', 'Connect to serial device\nParse framed telemetry with resync logic\nUpdate gauges and status chips\nSend operator commands for fan/manual actions\nExpose data path to Jetson analytics', {
        fontSize: 21,
      }),
      text({
        id: 'slide-6-note',
        name: 'Flow Note',
        x: 96,
        y: 772,
        w: 1320,
        h: 50,
        zIndex: 4,
        text: 'These flow blocks are fully editable in the collaborative editor and can be rearranged slide-side without touching the source HTML.',
        style: baseTextStyle({
          fontSize: 18,
          color: theme.colors.textSecondary,
          align: 'center',
        }),
      }),
    ],
  },
  {
    id: 'slide-7',
    name: 'Technical Challenges',
    background: defaultBackground,
    elements: [
      ...chrome(7, 'Challenges', 'Solutions'),
      text({
        id: 'slide-7-title',
        name: 'Slide Title',
        x: 68,
        y: 128,
        w: 780,
        h: 60,
        zIndex: 4,
        text: 'Technical Challenges',
        style: baseTextStyle({
          fontFamily: theme.fontDisplay,
          fontSize: 50,
          fontWeight: 800,
          color: theme.colors.textPrimary,
        }),
      }),
      text({
        id: 'slide-7-subhead',
        name: 'Slide Subhead',
        x: 72,
        y: 194,
        w: 820,
        h: 36,
        zIndex: 4,
        text: 'Key integration challenges across sensing, firmware, communication, and packaging.',
        style: baseTextStyle({
          fontSize: 20,
          color: 'rgba(255,255,255,0.86)',
        }),
      }),
      ...panel(7, 'challenge-1', 70, 286, 448, 476, 'Signal Integrity and Calibration', 'Current measurement scaling and thermistor interpretation needed careful verification\nVoltage visibility had to remain stable across the full 10-cell stack\nThe collaborative editor must preserve exact values without manual HTML edits', {
        fontSize: 21,
      }),
      ...panel(7, 'challenge-2', 576, 286, 448, 476, 'Firmware and Host Synchronization', 'USB CDC messaging needed framing, timeout handling, and parser recovery\nManual and automatic fan control paths had to coexist without undefined state\nConcurrent text editing in the deck mirrors the same synchronization challenge', {
        fontSize: 21,
      }),
      ...panel(7, 'challenge-3', 1082, 286, 448, 476, 'Packaging and Thermal Tradeoffs', 'Enclosure fit-up, airflow, cable routing, and serviceability competed for the same physical space\n3D assets needed to stay available inside the presentation while remaining editable on the web', {
        fontSize: 21,
      }),
    ],
  },
  {
    id: 'slide-8',
    name: 'Components and Implementation',
    background: defaultBackground,
    elements: [
      ...chrome(8, 'Implementation', 'Components'),
      text({
        id: 'slide-8-title',
        name: 'Slide Title',
        x: 68,
        y: 128,
        w: 1080,
        h: 60,
        zIndex: 4,
        text: 'Major Components of the Design and Implementation',
        style: baseTextStyle({
          fontFamily: theme.fontDisplay,
          fontSize: 44,
          fontWeight: 800,
          color: theme.colors.textPrimary,
        }),
      }),
      text({
        id: 'slide-8-subhead',
        name: 'Slide Subhead',
        x: 72,
        y: 194,
        w: 700,
        h: 36,
        zIndex: 4,
        text: 'Primary implementation ownership per subsystem.',
        style: baseTextStyle({
          fontSize: 20,
          color: 'rgba(255,255,255,0.86)',
        }),
      }),
      shape({
        id: 'slide-8-grid-a',
        name: 'Battery Pack Card',
        x: 68,
        y: 274,
        w: 350,
        h: 502,
        zIndex: 1,
        fill: cardFill,
        borderColor: thinBorder,
        borderWidth: 1,
        radius: 26,
        shadow: theme.shadow,
        opacity: 1,
      }),
      shape({
        id: 'slide-8-grid-b',
        name: 'BMS Card',
        x: 440,
        y: 274,
        w: 350,
        h: 502,
        zIndex: 1,
        fill: cardFill,
        borderColor: thinBorder,
        borderWidth: 1,
        radius: 26,
        shadow: theme.shadow,
        opacity: 1,
      }),
      shape({
        id: 'slide-8-grid-c',
        name: 'Firmware Card',
        x: 812,
        y: 274,
        w: 350,
        h: 502,
        zIndex: 1,
        fill: cardFill,
        borderColor: thinBorder,
        borderWidth: 1,
        radius: 26,
        shadow: theme.shadow,
        opacity: 1,
      }),
      shape({
        id: 'slide-8-grid-d',
        name: 'Dashboard Card',
        x: 1184,
        y: 274,
        w: 350,
        h: 502,
        zIndex: 1,
        fill: cardFill,
        borderColor: thinBorder,
        borderWidth: 1,
        radius: 26,
        shadow: theme.shadow,
        opacity: 1,
      }),
      image({
        id: 'slide-8-pack',
        name: 'Battery Pack Image',
        x: 88,
        y: 294,
        w: 310,
        h: 216,
        zIndex: 4,
        src: 'E-Load Image.png',
        alt: 'Battery pack',
        fit: 'contain',
        caption: 'Battery pack hardware',
        background: 'rgba(0,0,0,0.16)',
        borderColor: 'transparent',
        borderWidth: 0,
        radius: 18,
        shadow: 'none',
        preserveAspectRatio: true,
      }),
      image({
        id: 'slide-8-bms',
        name: 'BMS Image',
        x: 460,
        y: 294,
        w: 310,
        h: 216,
        zIndex: 4,
        src: 'BMS_Image.png',
        alt: 'BMS enclosure',
        fit: 'contain',
        caption: 'BMS enclosure',
        background: 'rgba(0,0,0,0.16)',
        borderColor: 'transparent',
        borderWidth: 0,
        radius: 18,
        shadow: 'none',
        preserveAspectRatio: true,
      }),
      image({
        id: 'slide-8-fw',
        name: 'Firmware Image',
        x: 832,
        y: 294,
        w: 310,
        h: 216,
        zIndex: 4,
        src: 'firmware.png',
        alt: 'Firmware development',
        fit: 'contain',
        caption: 'Firmware bring-up',
        background: 'rgba(0,0,0,0.16)',
        borderColor: 'transparent',
        borderWidth: 0,
        radius: 18,
        shadow: 'none',
        preserveAspectRatio: true,
      }),
      image({
        id: 'slide-8-gui',
        name: 'Dashboard Image',
        x: 1204,
        y: 294,
        w: 310,
        h: 216,
        zIndex: 4,
        src: 'GUI_Dashboard.png',
        alt: 'Dashboard UI',
        fit: 'contain',
        caption: 'Desktop dashboard',
        background: 'rgba(0,0,0,0.16)',
        borderColor: 'transparent',
        borderWidth: 0,
        radius: 18,
        shadow: 'none',
        preserveAspectRatio: true,
      }),
      ...panel(8, 'pack-panel', 88, 528, 310, 228, 'Battery Pack', '10s1p cell holder\nEnclosure fit-up\nHarness routing'),
      ...panel(8, 'bms-panel', 460, 528, 310, 228, 'BMS Board', 'BQ76930 monitoring\nProtection interfaces\nLayout integration'),
      ...panel(8, 'fw-panel', 832, 528, 310, 228, 'Firmware', 'STM32F303RC bring-up\nADC and sensing integration\nFan and USB CDC control'),
      ...panel(8, 'gui-panel', 1204, 528, 310, 228, 'Dashboard', 'PyQt6 launcher and bridge\nCommand-surface GUI\nJetson data path'),
    ],
  },
  {
    id: 'slide-9',
    name: 'Contributions',
    background: defaultBackground,
    elements: [
      ...chrome(9, 'Ownership', 'Team'),
      text({
        id: 'slide-9-title',
        name: 'Slide Title',
        x: 68,
        y: 128,
        w: 780,
        h: 60,
        zIndex: 4,
        text: 'Who Contributed to What',
        style: baseTextStyle({
          fontFamily: theme.fontDisplay,
          fontSize: 48,
          fontWeight: 800,
          color: theme.colors.textPrimary,
        }),
      }),
      text({
        id: 'slide-9-subhead',
        name: 'Slide Subhead',
        x: 72,
        y: 194,
        w: 700,
        h: 36,
        zIndex: 4,
        text: 'Subsystem leadership across the team.',
        style: baseTextStyle({
          fontSize: 20,
          color: 'rgba(255,255,255,0.86)',
        }),
      }),
      ...panel(9, 'member-1', 70, 286, 335, 476, 'Kaushik Vada', 'Deck system and collaborative editor\nWeb presentation design\nDashboard alignment and integration notes', {
        fontSize: 21,
      }),
      ...panel(9, 'member-2', 425, 286, 335, 476, 'Joshua Jensen', 'Battery hardware and system assembly\nMechanical integration and packaging\nTest readiness coordination', {
        fontSize: 21,
      }),
      ...panel(9, 'member-3', 780, 286, 335, 476, 'Hector Valladares', 'Embedded firmware runtime\nTelemetry framing and command handling\nFan control and sensing path', {
        fontSize: 21,
      }),
      ...panel(9, 'member-4', 1135, 286, 335, 476, 'Shared Deliverables', 'System architecture definition\nBench test execution\nPresentation and report integration', {
        fontSize: 21,
      }),
    ],
  },
  {
    id: 'slide-10',
    name: 'Design Considerations',
    background: defaultBackground,
    elements: [
      ...chrome(10, 'Standards and Constraints', 'Standards'),
      text({
        id: 'slide-10-title',
        name: 'Slide Title',
        x: 68,
        y: 128,
        w: 860,
        h: 60,
        zIndex: 4,
        text: 'Design Considerations',
        style: baseTextStyle({
          fontFamily: theme.fontDisplay,
          fontSize: 50,
          fontWeight: 800,
          color: theme.colors.textPrimary,
        }),
      }),
      text({
        id: 'slide-10-subhead',
        name: 'Slide Subhead',
        x: 72,
        y: 194,
        w: 920,
        h: 70,
        zIndex: 4,
        text: 'System shaped by schedule, thermal, and packaging limits\nRobust and well-understood communication standards kept the bring-up process tractable',
        style: baseTextStyle({
          fontSize: 20,
          bullet: true,
        }),
      }),
      ...panel(10, 'comm', 70, 286, 448, 228, 'Communication Robustness', 'Tolerant serial framing\nSimulation mode\nStaged bring-up for host-device loops', {
        fontSize: 20,
      }),
      ...panel(10, 'thermal', 546, 286, 448, 228, 'Thermal Control Decisions', '1 kHz PWM fan path\nBreakpoint auto-curve\n2°C hysteresis with clear status visibility', {
        fontSize: 20,
      }),
      ...panel(10, 'packaging', 1022, 286, 448, 228, 'Packaging Constraints', 'Compact enclosure with optimized airflow\nIterative 3D-printed revisions\nCable routing and serviceability tradeoffs', {
        fontSize: 20,
      }),
      shape({
        id: 'slide-10-standards-bg',
        name: 'Standards Panel',
        x: 70,
        y: 540,
        w: 1400,
        h: 232,
        zIndex: 1,
        fill: glassFill,
        borderColor: thinBorder,
        borderWidth: 1,
        radius: 30,
        shadow: theme.shadow,
        opacity: 1,
      }),
      text({
        id: 'slide-10-standards-title',
        name: 'Standards Title',
        x: 98,
        y: 574,
        w: 600,
        h: 42,
        zIndex: 4,
        text: 'Industry Standards and Interfaces Used',
        style: baseTextStyle({
          fontFamily: theme.fontDisplay,
          fontSize: 30,
          fontWeight: 800,
          color: theme.colors.textPrimary,
        }),
      }),
      text({
        id: 'slide-10-standards-body',
        name: 'Standards Body',
        x: 102,
        y: 632,
        w: 1240,
        h: 100,
        zIndex: 4,
        text: 'USB CDC for host communication\nPWM for cooling actuation\nSTM32Cube and HAL stack reuse\nSensor scaling anchored to schematic constants and firmware conversion factors',
        style: baseTextStyle({
          fontSize: 22,
          bullet: true,
          lineHeight: 1.4,
        }),
      }),
    ],
  },
  {
    id: 'slide-11',
    name: 'Test Report',
    background: defaultBackground,
    elements: [
      ...chrome(11, 'Validation', 'Results'),
      text({
        id: 'slide-11-title',
        name: 'Slide Title',
        x: 68,
        y: 128,
        w: 760,
        h: 60,
        zIndex: 4,
        text: 'Test Report',
        style: baseTextStyle({
          fontFamily: theme.fontDisplay,
          fontSize: 52,
          fontWeight: 800,
          color: theme.colors.textPrimary,
        }),
      }),
      text({
        id: 'slide-11-subhead',
        name: 'Slide Subhead',
        x: 72,
        y: 194,
        w: 920,
        h: 36,
        zIndex: 4,
        text: 'Validated behaviors based directly on repo documentation and implementation.',
        style: baseTextStyle({
          fontSize: 20,
          color: 'rgba(255,255,255,0.86)',
        }),
      }),
      table({
        id: 'slide-11-table',
        name: 'Validation Table',
        x: 70,
        y: 268,
        w: 930,
        h: 444,
        zIndex: 4,
        columns: ['Capability', 'Expected', 'Observed'],
        rows: [
          ['Cell telemetry', 'All 10 cells reported', 'Visible through monitor and UI'],
          ['Temperature telemetry', '10 NTC channels sampled', 'Tracked in firmware and dashboard'],
          ['Host comms', 'USB CDC command path', 'Commands and telemetry active'],
          ['Fan control', 'Auto and manual support', 'PWM path implemented'],
          ['Parser recovery', 'Resync after bad framing', 'Handled in dashboard bridge'],
        ],
        style: {
          headerBackground: 'rgba(10,132,255,0.2)',
          headerColor: theme.colors.textPrimary,
          cellBackground: 'rgba(255,255,255,0.06)',
          cellColor: theme.colors.textSecondary,
          borderColor: thinBorder,
          accentColor: theme.colors.accent,
          fontSize: 16,
        },
      }),
      ...panel(11, 'validation-methods', 1030, 268, 458, 138, 'Validation Methods', 'Firmware checks\nSerial reconstruction tests\nGUI verification\nStaged bench testing'),
      ...panel(11, 'spec-gap', 1030, 428, 458, 138, 'How Close to Spec?', 'Implemented: communications, telemetry, fan control, monitoring\nRemaining gap: calibration and live scale testing'),
      ...panel(11, 'anchors', 1030, 588, 458, 138, 'Quantitative Anchors', '10 cells and 10 NTCs\n115200 baud and 1 kHz PWM\n2°C hysteresis and 0.422 mA/LSB\n2s timeout and 50 ms fallback'),
    ],
  },
  {
    id: 'slide-12',
    name: 'Demo Placeholder',
    background: defaultBackground,
    elements: [
      ...chrome(12, 'Demo', 'Video'),
      text({
        id: 'slide-12-title',
        name: 'Slide Title',
        x: 68,
        y: 128,
        w: 760,
        h: 60,
        zIndex: 4,
        text: 'Demo Video Placeholder',
        style: baseTextStyle({
          fontFamily: theme.fontDisplay,
          fontSize: 48,
          fontWeight: 800,
          color: theme.colors.textPrimary,
        }),
      }),
      text({
        id: 'slide-12-subhead',
        name: 'Slide Subhead',
        x: 72,
        y: 194,
        w: 700,
        h: 36,
        zIndex: 4,
        text: 'Reserved space for the recorded demo clip.',
        style: baseTextStyle({
          fontSize: 20,
          color: 'rgba(255,255,255,0.86)',
        }),
      }),
      shape({
        id: 'slide-12-video-stage',
        name: 'Video Stage',
        x: 98,
        y: 274,
        w: 1080,
        h: 460,
        zIndex: 1,
        fill: glassFill,
        borderColor: thinBorder,
        borderWidth: 1,
        radius: 32,
        shadow: theme.shadow,
        opacity: 1,
      }),
      text({
        id: 'slide-12-video-placeholder',
        name: 'Video Placeholder Copy',
        x: 220,
        y: 414,
        w: 840,
        h: 150,
        zIndex: 4,
        text: 'Insert final demo video here\nSequence: connect USB, stream telemetry, toggle fan, show status, demonstrate operator interaction',
        style: baseTextStyle({
          fontSize: 28,
          fontWeight: 700,
          align: 'center',
          color: theme.colors.textPrimary,
          lineHeight: 1.35,
        }),
      }),
      ...panel(12, 'scene-1', 1212, 274, 280, 104, 'Scene 1', 'Dashboard boot and serial connection', {
        fontSize: 17,
        bullet: false,
      }),
      ...panel(12, 'scene-2', 1212, 396, 280, 104, 'Scene 2', 'Live cell, current, and temperature telemetry', {
        fontSize: 17,
        bullet: false,
      }),
      ...panel(12, 'scene-3', 1212, 518, 280, 104, 'Scene 3', 'Fan auto and manual control with status updates', {
        fontSize: 17,
        bullet: false,
      }),
      ...panel(12, 'scene-4', 1212, 640, 280, 104, 'Scene 4', 'E-Load or charger interaction and system response', {
        fontSize: 17,
        bullet: false,
      }),
    ],
  },
  {
    id: 'slide-13',
    name: 'Summary',
    background: defaultBackground,
    elements: [
      ...chrome(13, 'Summary', 'Next Phase'),
      text({
        id: 'slide-13-title',
        name: 'Slide Title',
        x: 68,
        y: 128,
        w: 760,
        h: 60,
        zIndex: 4,
        text: 'Summary',
        style: baseTextStyle({
          fontFamily: theme.fontDisplay,
          fontSize: 54,
          fontWeight: 800,
          color: theme.colors.textPrimary,
        }),
      }),
      text({
        id: 'slide-13-quote',
        name: 'Summary Quote',
        x: 72,
        y: 208,
        w: 660,
        h: 120,
        zIndex: 4,
        text: 'Delivered a cross-domain engineering system with a path toward AI analytics.',
        style: baseTextStyle({
          fontFamily: theme.fontDisplay,
          fontSize: 34,
          fontWeight: 700,
          color: theme.colors.textPrimary,
          background: glassFill,
          borderColor: thinBorder,
          borderWidth: 1,
          radius: 28,
          padding: 20,
          lineHeight: 1.22,
        }),
      }),
      ...panel(13, 'achieved', 72, 364, 210, 182, 'What Was Achieved', 'BQ76930 sensing and STM32 control\nUSB telemetry and premium dashboard\nValidated architecture', {
        fontSize: 16,
      }),
      ...panel(13, 'strongest', 302, 364, 210, 182, 'What Is Strongest', 'Cell visibility and current measurement\nThermal control\nParser robustness and GUI usability', {
        fontSize: 16,
      }),
      ...panel(13, 'next', 532, 364, 210, 182, 'What Comes Next', 'Final calibration and charger validation\nFault handling and trending\nJetson-based state estimation', {
        fontSize: 16,
      }),
      shape({
        id: 'slide-13-image-bg',
        name: 'Dashboard Frame',
        x: 790,
        y: 208,
        w: 706,
        h: 402,
        zIndex: 1,
        fill: cardFill,
        borderColor: thinBorder,
        borderWidth: 1,
        radius: 28,
        shadow: theme.shadow,
        opacity: 1,
      }),
      image({
        id: 'slide-13-gui',
        name: 'Final Dashboard Image',
        x: 816,
        y: 236,
        w: 654,
        h: 322,
        zIndex: 4,
        src: 'GUI_Dashboard.png',
        alt: 'Final dashboard',
        fit: 'contain',
        caption: 'Final command surface preserving the project visual language',
        background: 'rgba(0,0,0,0.16)',
        borderColor: 'transparent',
        borderWidth: 0,
        radius: 18,
        shadow: 'none',
        preserveAspectRatio: true,
      }),
      ...panel(13, 'takeaway', 790, 636, 706, 134, 'Closing Takeaway', 'A modular, safe, expandable battery platform ready for analytics and further validation.', {
        bullet: false,
        fontSize: 22,
      }),
      text({
        id: 'slide-13-footnote',
        name: 'Summary Footnote',
        x: 76,
        y: 592,
        w: 664,
        h: 120,
        zIndex: 4,
        text: 'Sources used in this deck: final report, weekly reports, firmware plans, dashboard source, schematic captures, and project assets from the EE175 workspace.',
        style: baseTextStyle({
          fontSize: 15,
          color: theme.colors.textMuted,
          lineHeight: 1.45,
        }),
      }),
    ],
  },
]

export const seedDeck: DeckDocument = {
  id: DECK_ID,
  title: 'BMS Final Design Collaborative Deck',
  size: {
    width: SLIDE_WIDTH,
    height: SLIDE_HEIGHT,
  },
  theme,
  slides: seededSlides,
}

export const cloneElement = <T extends SlideElement>(element: T, nextId: string): T => {
  const copy = JSON.parse(JSON.stringify(element)) as T
  copy.id = nextId
  copy.name = `${element.name} Copy`
  copy.x += 28
  copy.y += 28
  copy.zIndex += 1
  return copy
}
