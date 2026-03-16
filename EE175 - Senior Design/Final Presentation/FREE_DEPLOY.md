# Free Deployment

This project now supports a no-cost deployment split:

- GitHub Pages serves the React editor UI.
- A Cloudflare Worker + Durable Object stores deck progress and handles live collaboration.

## What already works

- All seeded deck assets are copied into the Pages build automatically.
- Edits and snapshots persist in the Durable Object storage.
- Realtime collaboration runs over WebSockets through the Worker.
- Uploading new files is intentionally disabled on the free deployment path.

## One-time setup

1. In GitHub repo settings, enable **Pages** and set the source to **GitHub Actions**.
2. Create a free Cloudflare account.
3. In Cloudflare, create an API token that can deploy Workers.
4. In GitHub repo secrets, add:
   - `CLOUDFLARE_API_TOKEN`
   - `CLOUDFLARE_ACCOUNT_ID`
5. Push to `main`.

## What deploys where

- `.github/workflows/deploy-final-presentation-pages.yml`
  - Builds `EE175 - Senior Design/Final Presentation/app`
  - Publishes the static site to GitHub Pages

- `.github/workflows/deploy-final-presentation-worker.yml`
  - Deploys `EE175 - Senior Design/Final Presentation/worker`
  - Creates the `bms-collaborative-deck` Worker/Durable Object backend

## Share link format

After the Worker deploys, Cloudflare gives you a `workers.dev` URL. Share the GitHub Pages URL with that backend attached as a query parameter:

```text
https://kaushikvada3.github.io/UCR-Computer-Science-Electrical-Engineering/?api=https://YOUR-WORKER.YOUR-SUBDOMAIN.workers.dev
```

The app stores that backend URL in local storage after the first visit, so you do not need to append `?api=` every time on the same browser. For new collaborators, share the full URL above.

## Local free-stack dev

Run the Worker locally:

```bash
npm --prefix worker run dev
```

In a second terminal, run the app against that local Worker:

```bash
cd app
VITE_DECK_API_BASE_URL=http://127.0.0.1:8788 VITE_DECK_WS_BASE_URL=ws://127.0.0.1:8788 npm run dev
```
