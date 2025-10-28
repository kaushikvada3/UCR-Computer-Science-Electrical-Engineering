import * as THREE from "https://cdn.jsdelivr.net/npm/three@0.160/build/three.module.js";
import { OrbitControls } from "https://cdn.jsdelivr.net/npm/three@0.160/examples/jsm/controls/OrbitControls.js";

const { gsap } = window;

const CELL_COUNT = 10;
const CELL_ROWS = 2;
const CELLS_PER_ROW = CELL_COUNT / CELL_ROWS;
const CELL_HEIGHT = 2.6;
const CELL_RADIUS = 0.42;
const CELL_SPACING_X = 1.45;
const CELL_SPACING_Z = 1.2;
const PACK_LENGTH = (CELLS_PER_ROW - 1) * CELL_SPACING_X + 3.4;
const PACK_WIDTH = (CELL_ROWS - 1) * CELL_SPACING_Z + 1.9;
const BASE_THICKNESS = 0.35;

const canvas = document.getElementById("scene");
const renderer = new THREE.WebGLRenderer({
  canvas,
  antialias: true,
  alpha: true,
});
renderer.shadowMap.enabled = true;
renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
renderer.setSize(window.innerWidth, window.innerHeight);

const scene = new THREE.Scene();
scene.background = new THREE.Color(0x020406);
scene.fog = new THREE.Fog(0x020406, 35, 80);

const camera = new THREE.PerspectiveCamera(
  45,
  window.innerWidth / window.innerHeight,
  0.1,
  200
);
camera.position.set(10, 9, 16);

const controls = new OrbitControls(camera, renderer.domElement);
controls.enableDamping = true;
controls.dampingFactor = 0.05;
controls.enablePan = false;
controls.maxPolarAngle = Math.PI / 2.05;
controls.target.set(0, 2, 0);

const hemiLight = new THREE.HemisphereLight(0xe1ecff, 0x0a0a0a, 0.3);
scene.add(hemiLight);

const keyLight = new THREE.DirectionalLight(0xffffff, 0.9);
keyLight.position.set(-6, 12, 7);
keyLight.castShadow = true;
keyLight.shadow.mapSize.set(1024, 1024);
scene.add(keyLight);

const rimLight = new THREE.DirectionalLight(0x7fd4ff, 0.5);
rimLight.position.set(10, 8, -6);
scene.add(rimLight);

const floor = new THREE.Mesh(
  new THREE.CircleGeometry(40, 64),
  new THREE.MeshStandardMaterial({
    color: 0x030405,
    roughness: 0.9,
    metalness: 0.1,
    transparent: true,
    opacity: 0.85,
  })
);
floor.rotation.x = -Math.PI / 2;
floor.receiveShadow = true;
scene.add(floor);

const assemblyGroup = new THREE.Group();
scene.add(assemblyGroup);

const casingMaterial = new THREE.MeshStandardMaterial({
  color: 0xb6ae9b,
  metalness: 0.35,
  roughness: 0.55,
});
const accentMaterial = new THREE.MeshStandardMaterial({
  color: 0x867a67,
  metalness: 0.3,
  roughness: 0.6,
});
const bridgeMaterial = new THREE.MeshStandardMaterial({
  color: 0x918678,
  metalness: 0.3,
  roughness: 0.45,
});
const cellBodyBase = new THREE.MeshStandardMaterial({
  color: 0x7d8ea3,
  metalness: 0.18,
  roughness: 0.35,
});
const cellCapBase = new THREE.MeshStandardMaterial({
  color: 0xe6e8ec,
  metalness: 0.8,
  roughness: 0.2,
  emissive: new THREE.Color(0x162b40),
  emissiveIntensity: 0.3,
});
const fanFrameMaterial = new THREE.MeshStandardMaterial({
  color: 0xa9a08f,
  metalness: 0.4,
  roughness: 0.45,
});
const fanRotorMaterial = new THREE.MeshStandardMaterial({
  color: 0x6fb7ff,
  metalness: 0.25,
  roughness: 0.25,
  emissive: new THREE.Color(0x0b3a55),
  emissiveIntensity: 0.45,
});

const cellMeshes = [];
let fanRotor = null;
let highlightedCellId = null;

buildBatteryModule();
buildFanAssembly();

function buildBatteryModule() {
  const packGroup = new THREE.Group();
  assemblyGroup.add(packGroup);

  const base = new THREE.Mesh(
    new THREE.BoxGeometry(PACK_LENGTH, BASE_THICKNESS, PACK_WIDTH),
    casingMaterial
  );
  base.position.y = BASE_THICKNESS / 2;
  base.castShadow = base.receiveShadow = true;
  packGroup.add(base);

  const topRailGeo = new THREE.BoxGeometry(PACK_LENGTH - 0.3, 0.25, 0.7);
  const railOffsets = [-PACK_WIDTH / 2 + 0.5, PACK_WIDTH / 2 - 0.5];
  railOffsets.forEach((z) => {
    const rail = new THREE.Mesh(topRailGeo, casingMaterial);
    rail.position.set(0, BASE_THICKNESS + CELL_HEIGHT + 0.15, z);
    rail.castShadow = true;
    packGroup.add(rail);
  });

  const supportGeo = new THREE.BoxGeometry(0.4, CELL_HEIGHT + 0.6, 0.4);
  const supportPositions = [
    [-PACK_LENGTH / 2 + 0.4, (CELL_HEIGHT + BASE_THICKNESS) / 2, -PACK_WIDTH / 2 + 0.4],
    [PACK_LENGTH / 2 - 0.4, (CELL_HEIGHT + BASE_THICKNESS) / 2, -PACK_WIDTH / 2 + 0.4],
    [-PACK_LENGTH / 2 + 0.4, (CELL_HEIGHT + BASE_THICKNESS) / 2, PACK_WIDTH / 2 - 0.4],
    [PACK_LENGTH / 2 - 0.4, (CELL_HEIGHT + BASE_THICKNESS) / 2, PACK_WIDTH / 2 - 0.4],
  ];
  supportPositions.forEach(([x, y, z]) => {
    const support = new THREE.Mesh(supportGeo, accentMaterial);
    support.position.set(x, y, z);
    support.castShadow = support.receiveShadow = true;
    packGroup.add(support);
  });

  const midBrace = new THREE.Mesh(
    new THREE.BoxGeometry(0.5, CELL_HEIGHT + 0.4, PACK_WIDTH - 0.6),
    accentMaterial
  );
  midBrace.position.set(0, (CELL_HEIGHT + BASE_THICKNESS) / 2, 0);
  midBrace.castShadow = true;
  packGroup.add(midBrace);

  let id = 1;
  for (let row = 0; row < CELL_ROWS; row += 1) {
    for (let col = 0; col < CELLS_PER_ROW; col += 1) {
      const cellGroup = new THREE.Group();
      const x = (col - (CELLS_PER_ROW - 1) / 2) * CELL_SPACING_X;
      const z = (row - (CELL_ROWS - 1) / 2) * CELL_SPACING_Z;
      const anchorY = BASE_THICKNESS + CELL_HEIGHT / 2;
      cellGroup.position.set(x, anchorY, z);
      cellGroup.userData.id = id;

      const bodyMaterial = cellBodyBase.clone();
      const body = new THREE.Mesh(
        new THREE.CylinderGeometry(CELL_RADIUS, CELL_RADIUS, CELL_HEIGHT, 48),
        bodyMaterial
      );
      body.castShadow = body.receiveShadow = true;

      const capMaterial = cellCapBase.clone();
      const cap = new THREE.Mesh(
        new THREE.CylinderGeometry(CELL_RADIUS * 0.55, CELL_RADIUS * 0.55, 0.14, 32),
        capMaterial
      );
      cap.position.y = CELL_HEIGHT / 2 + 0.08;
      cap.castShadow = true;

      const collar = new THREE.Mesh(
        new THREE.CylinderGeometry(CELL_RADIUS * 1.05, CELL_RADIUS * 1.05, 0.2, 32),
        accentMaterial
      );
      collar.position.y = -CELL_HEIGHT / 2 + 0.1;

      cellGroup.add(body, cap, collar);
      packGroup.add(cellGroup);

      cellMeshes.push({
        id,
        group: cellGroup,
        body,
        indicator: cap,
        anchorY,
      });

      id += 1;
    }
  }
}

function buildFanAssembly() {
  const fanGroup = new THREE.Group();
  fanGroup.position.set(PACK_LENGTH / 2 + 2.6, BASE_THICKNESS + 1.3, 0);
  assemblyGroup.add(fanGroup);

  const bridge = new THREE.Mesh(
    new THREE.BoxGeometry(2.4, 0.35, PACK_WIDTH * 0.7),
    bridgeMaterial
  );
  bridge.position.set(PACK_LENGTH / 2, BASE_THICKNESS + 0.6, 0);
  bridge.castShadow = bridge.receiveShadow = true;
  assemblyGroup.add(bridge);

  const bridgeSupport = new THREE.Mesh(
    new THREE.BoxGeometry(0.35, 2.1, PACK_WIDTH * 0.5),
    accentMaterial
  );
  bridgeSupport.position.set(PACK_LENGTH / 2 + 1.1, BASE_THICKNESS + 1, 0);
  bridgeSupport.castShadow = true;
  assemblyGroup.add(bridgeSupport);

  const frameGeo = new THREE.BoxGeometry(4.3, 4.3, 0.35);
  const frontFrame = new THREE.Mesh(frameGeo, fanFrameMaterial);
  frontFrame.position.x = 0.35;
  const backFrame = frontFrame.clone();
  backFrame.position.x = -0.35;
  fanGroup.add(frontFrame, backFrame);

  const shroud = new THREE.Mesh(
    new THREE.BoxGeometry(4.2, 4.2, 0.5),
    fanFrameMaterial
  );
  shroud.castShadow = true;
  fanGroup.add(shroud);

  const strutGeo = new THREE.BoxGeometry(0.25, 4.2, 0.12);
  [-0.8, 0.8].forEach((z) => {
    const strut = new THREE.Mesh(strutGeo, fanFrameMaterial);
    strut.position.set(0, 0, z);
    fanGroup.add(strut);
  });

  fanRotor = new THREE.Group();
  fanGroup.add(fanRotor);

  const rotorRing = new THREE.Mesh(
    new THREE.TorusGeometry(1.4, 0.08, 24, 80),
    fanRotorMaterial
  );
  rotorRing.rotation.y = Math.PI / 2;
  fanRotor.add(rotorRing);

  const hub = new THREE.Mesh(
    new THREE.CylinderGeometry(0.35, 0.35, 0.4, 32),
    fanRotorMaterial
  );
  hub.rotation.z = Math.PI / 2;
  fanRotor.add(hub);

  const bladeGeo = new THREE.BoxGeometry(0.2, 0.05, 2.6);
  bladeGeo.translate(0, 0, 1.3);
  for (let i = 0; i < 4; i += 1) {
    const blade = new THREE.Mesh(bladeGeo, fanRotorMaterial);
    blade.rotation.y = (Math.PI / 2) * i;
    fanRotor.add(blade);
  }

  const fanGlow = new THREE.PointLight(0x5dc6ff, 1.6, 8);
  fanGlow.position.set(
    PACK_LENGTH / 2 + 2.2,
    BASE_THICKNESS + 1.6,
    0
  );
  assemblyGroup.add(fanGlow);
}

const packVoltageEl = document.querySelector("[data-pack-voltage]");
const packTempEl = document.querySelector("[data-pack-temp]");
const fanSpeedEl = document.querySelector("[data-fan-speed]");
const modeEl = document.querySelector("[data-pack-mode]");
const thermalTrendEl = document.querySelector("[data-thermal-trend]");
const cellGridEl = document.querySelector(".cell-grid");
const detailPanel = document.querySelector("[data-detail-panel]");
const detailTitle = document.querySelector("[data-cell-title]");
const detailVoltage = document.querySelector("[data-cell-voltage]");
const detailTemp = document.querySelector("[data-cell-temperature]");
const detailDelta = document.querySelector("[data-cell-delta]");
const closePanelBtn = document.querySelector("[data-close-panel]");

let currentState = null;

const state = {
  cells: Array.from({ length: CELL_COUNT }, (_, i) => ({
    id: i + 1,
    voltage: 3.8 + Math.random() * 0.4,
    temperature: 28 + Math.random() * 6,
  })),
  mode: "Auto-Balance",
  fan: { rpm: 1200 },
};

function populateCellGrid() {
  const fragment = document.createDocumentFragment();
  state.cells.forEach((cell) => {
    const card = document.createElement("button");
    card.className = "cell-card";
    card.type = "button";
    card.innerHTML = `
      <p class="cell-card__title">Cell ${cell.id.toString().padStart(2, "0")}</p>
      <p class="cell-card__value">${cell.voltage.toFixed(2)} V</p>
      <p class="cell-card__temp">${cell.temperature.toFixed(1)} °C</p>
    `;
    card.dataset.cellId = cell.id;
    card.addEventListener("click", () => {
      showDetail(cell.id);
      highlightCell(cell.id);
    });
    fragment.appendChild(card);
  });
  cellGridEl.innerHTML = "";
  cellGridEl.appendChild(fragment);
}

function showDetail(cellId) {
  const cell = currentState?.cells.find((c) => c.id === cellId);
  if (!cell) return;
  detailTitle.textContent = `Cell ${cell.id.toString().padStart(2, "0")}`;
  detailVoltage.textContent = `${cell.voltage.toFixed(3)} V`;
  detailTemp.textContent = `${cell.temperature.toFixed(1)} °C`;
  detailDelta.textContent = `${((cell.voltage - 3.8) * 1000).toFixed(0)} mV`;
  detailPanel.classList.add("is-visible");
}

closePanelBtn.addEventListener("click", () => {
  detailPanel.classList.remove("is-visible");
  highlightCell(null);
});

function highlightCell(cellId) {
  highlightedCellId = cellId;
  cellMeshes.forEach((cell) => {
    const active = cellId != null && cell.id === cellId;
    gsap.to(cell.group.scale, {
      x: active ? 1.06 : 1,
      y: active ? 1.06 : 1,
      z: active ? 1.06 : 1,
      duration: 0.4,
      ease: "power2.out",
    });
    gsap.to(cell.indicator.material, {
      emissiveIntensity: active ? 1.3 : 0.35,
      duration: 0.5,
      ease: "power2.out",
    });
  });
}

function updateHud(data) {
  const packVoltage = data.cells.reduce((acc, cell) => acc + cell.voltage, 0);
  const peakTemp = Math.max(...data.cells.map((cell) => cell.temperature));
  packVoltageEl.textContent = `${packVoltage.toFixed(1)} V`;
  packTempEl.textContent = `${peakTemp.toFixed(1)} °C`;
  fanSpeedEl.textContent = `${Math.round(data.fan.rpm)} RPM`;
  modeEl.textContent = data.mode;
  thermalTrendEl.textContent =
    peakTemp > 38 ? "Cooling engaged" : "Holding steady";

  document.querySelectorAll(".cell-card").forEach((card) => {
    const id = Number(card.dataset.cellId);
    const cell = data.cells.find((c) => c.id === id);
    if (!cell) return;
    card.querySelector(".cell-card__value").textContent = `${cell.voltage.toFixed(
      2
    )} V`;
    card.querySelector(".cell-card__temp").textContent = `${cell.temperature.toFixed(
      1
    )} °C`;
  });
}

function colorForVoltage(voltage) {
  const normalized = THREE.MathUtils.clamp((voltage - 3.2) / (4.2 - 3.2), 0, 1);
  const hue = THREE.MathUtils.lerp(0, 0.38, normalized);
  const color = new THREE.Color();
  color.setHSL(hue, 0.7, 0.5);
  return color;
}

function animateCells(data) {
  data.cells.forEach((cellData) => {
    const mesh = cellMeshes.find((entry) => entry.id === cellData.id);
    if (!mesh) return;
    const voltageColor = colorForVoltage(cellData.voltage);
    gsap.to(mesh.body.material.color, {
      r: voltageColor.r,
      g: voltageColor.g,
      b: voltageColor.b,
      duration: 0.6,
      ease: "power2.out",
    });

    const tempColor = new THREE.Color();
    tempColor.setHSL(
      THREE.MathUtils.mapLinear(cellData.temperature, 27, 45, 0.08, 0),
      0.8,
      0.55
    );
    gsap.to(mesh.indicator.material.color, {
      r: tempColor.r,
      g: tempColor.g,
      b: tempColor.b,
      duration: 0.6,
      ease: "power2.out",
    });

    const lift = THREE.MathUtils.mapLinear(cellData.temperature, 27, 45, 0, 0.22);
    gsap.to(mesh.group.position, {
      y: mesh.anchorY + lift,
      duration: 0.8,
      ease: "sine.out",
    });
  });
}

const clock = new THREE.Clock();
let elapsedTime = 0;

function tick() {
  const delta = clock.getDelta();
  elapsedTime += delta;
  controls.update();

  assemblyGroup.rotation.y += delta * 0.15;
  assemblyGroup.position.y = Math.sin(elapsedTime * 0.8) * 0.08;

  if (fanRotor && currentState) {
    fanRotor.rotation.x += delta * (currentState.fan.rpm / 60) * Math.PI * 0.4;
  }

  renderer.render(scene, camera);
  requestAnimationFrame(tick);
}

tick();

function mockStream() {
  const nextCells = state.cells.map((cell) => {
    const voltage = THREE.MathUtils.clamp(
      cell.voltage + THREE.MathUtils.randFloatSpread(0.035),
      3.35,
      4.18
    );
    const temperature = THREE.MathUtils.clamp(
      cell.temperature + THREE.MathUtils.randFloatSpread(0.9),
      27,
      45
    );
    return { ...cell, voltage, temperature };
  });

  const rpm =
    1200 +
    Math.sin(Date.now() * 0.0012) * 220 +
    THREE.MathUtils.randFloatSpread(60);
  const mode = Math.random() > 0.82 ? "Balancing" : "Auto-Balance";

  currentState = {
    ...state,
    cells: nextCells,
    fan: { rpm },
    mode,
  };

  state.cells = nextCells;

  updateHud(currentState);
  animateCells(currentState);
}

populateCellGrid();
mockStream();
setInterval(mockStream, 1500);

window.addEventListener("resize", () => {
  camera.aspect = window.innerWidth / window.innerHeight;
  camera.updateProjectionMatrix();
  renderer.setSize(window.innerWidth, window.innerHeight);
});
