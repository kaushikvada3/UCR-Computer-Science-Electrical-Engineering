import fs from 'node:fs/promises'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __filename = fileURLToPath(import.meta.url)
const __dirname = path.dirname(__filename)
const appRoot = path.resolve(__dirname, '..')
const projectRoot = path.resolve(appRoot, '..')
const targetDir = path.join(appRoot, 'public', 'deck-assets')

const assetFiles = [
  'BMS.glb',
  'BMS_Image.png',
  'E-Load.glb',
  'E-Load Image.png',
  'system_block_diagram.png',
  'bms_schematic_bq76930.png',
  'bms_schematic_stm32.png',
  'bms_schematic_fan.png',
  'firmware.png',
  'GUI_Dashboard.png',
]

const copyIfChanged = async (sourcePath, targetPath) => {
  const [sourceStats, targetStats] = await Promise.all([
    fs.stat(sourcePath),
    fs.stat(targetPath).catch(() => null),
  ])

  if (
    targetStats &&
    targetStats.size === sourceStats.size &&
    targetStats.mtimeMs >= sourceStats.mtimeMs
  ) {
    return
  }

  await fs.copyFile(sourcePath, targetPath)
}

await fs.mkdir(targetDir, { recursive: true })

await Promise.all(assetFiles.map(async (filename) => {
  const sourcePath = path.join(projectRoot, filename)
  const targetPath = path.join(targetDir, filename)
  await copyIfChanged(sourcePath, targetPath)
}))
