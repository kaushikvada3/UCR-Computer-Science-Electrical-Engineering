import type { ThemeTokens } from '../../shared/src/deck.js'
import {
  DECK_ID,
  SHARE_TOKEN,
  seedDeck,
} from '../../shared/src/deck.js'
import { DeckRoom } from './room'

export { DeckRoom }

type BootstrapPayload = {
  deckId: string
  roomId: string
  shareToken: string
  theme: ThemeTokens
  assetBaseUrl: string
  websocketPath: string
  collaboratorLabel: string
  uploadsEnabled: boolean
}

const corsHeaders = {
  'Access-Control-Allow-Origin': '*',
  'Access-Control-Allow-Methods': 'GET,POST,OPTIONS',
  'Access-Control-Allow-Headers': 'Content-Type',
  'Cache-Control': 'no-store',
}

const json = (payload: unknown, status = 200) => new Response(JSON.stringify(payload), {
  status,
  headers: {
    ...corsHeaders,
    'Content-Type': 'application/json',
  },
})

const withCors = async (response: Response) => {
  const headers = new Headers(response.headers)
  Object.entries(corsHeaders).forEach(([key, value]) => headers.set(key, value))
  return new Response(response.body, {
    status: response.status,
    statusText: response.statusText,
    headers,
  })
}

const bootstrapPayload = (): BootstrapPayload => ({
  deckId: DECK_ID,
  roomId: DECK_ID,
  shareToken: SHARE_TOKEN,
  theme: seedDeck.theme,
  assetBaseUrl: '/uploads',
  websocketPath: '/collab',
  collaboratorLabel: `Editor ${Math.floor(Math.random() * 900 + 100)}`,
  uploadsEnabled: false,
})

export interface Env {
  DECK_ROOM: DurableObjectNamespace
}

const getRoom = (env: Env, roomId = DECK_ID) => env.DECK_ROOM.get(env.DECK_ROOM.idFromName(roomId))

export default {
  async fetch(request: Request, env: Env): Promise<Response> {
    const url = new URL(request.url)

    if (request.method === 'OPTIONS') {
      return new Response(null, {
        status: 204,
        headers: corsHeaders,
      })
    }

    if (url.pathname === '/api/health') {
      return json({ ok: true, runtime: 'cloudflare-worker' })
    }

    if (request.method === 'GET' && url.pathname === `/api/share/${SHARE_TOKEN}/bootstrap`) {
      return json(bootstrapPayload())
    }

    if (url.pathname.startsWith('/api/share/')) {
      return json({ error: 'Unknown share link' }, 404)
    }

    if (url.pathname === `/collab/${DECK_ID}`) {
      return getRoom(env, DECK_ID).fetch(request)
    }

    if (request.method === 'GET' && url.pathname === `/api/decks/${DECK_ID}/history`) {
      const target = new URL('/internal/history', url)
      return withCors(await getRoom(env).fetch(new Request(target, request)))
    }

    if (request.method === 'POST' && url.pathname === `/api/decks/${DECK_ID}/publish-snapshot`) {
      const target = new URL('/internal/publish-snapshot', url)
      return withCors(await getRoom(env).fetch(new Request(target, request)))
    }

    if (request.method === 'POST' && url.pathname.startsWith(`/api/decks/${DECK_ID}/restore-snapshot/`)) {
      const snapshotId = url.pathname.slice(`/api/decks/${DECK_ID}/restore-snapshot/`.length)
      const target = new URL(`/internal/restore-snapshot/${snapshotId}`, url)
      return withCors(await getRoom(env).fetch(new Request(target, request)))
    }

    if (url.pathname.startsWith('/api/decks/')) {
      return json({ error: 'Unknown deck' }, 404)
    }

    return json({ error: 'Not found' }, 404)
  },
}
