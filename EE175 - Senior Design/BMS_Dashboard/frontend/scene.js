import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";
import { FBXLoader } from "three/addons/loaders/FBXLoader.js";

const { gsap } = window;

// --- Configuration ---
const MODEL_PATH = "battery_design.fbx";
const CELL_NAME_PATTERN = /cell|battery|cylinder/i; // Regex to find battery cells in the model

// --- Scene Setup ---
const canvas = document.getElementById("scene");
const renderer = new THREE.WebGLRenderer({
  canvas,
  antialias: true,
  alpha: true,
});
renderer.shadowMap.enabled = true;
renderer.shadowMap.type = THREE.PCFSoftShadowMap;
renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
renderer.setSize(window.innerWidth, window.innerHeight);
renderer.toneMapping = THREE.ACESFilmicToneMapping;
renderer.toneMappingExposure = 1.2;

const scene = new THREE.Scene();
// No background color - let CSS gradient show through
// scene.background = new THREE.Color(0x000000); 

const camera = new THREE.PerspectiveCamera(
  45,
  window.innerWidth / window.innerHeight,
  0.1,
  1000
);
camera.position.set(15, 12, 20);

const controls = new OrbitControls(camera, renderer.domElement);
controls.enableDamping = true;
controls.dampingFactor = 0.05;
controls.enablePan = false;
controls.maxPolarAngle = Math.PI / 2.1;
controls.target.set(0, 0, 0);

// --- Lighting ---
const ambientLight = new THREE.AmbientLight(0xffffff, 0.4);
scene.add(ambientLight);

const dirLight = new THREE.DirectionalLight(0xffffff, 1.5);
dirLight.position.set(10, 20, 10);
dirLight.castShadow = true;
dirLight.shadow.mapSize.width = 2048;
dirLight.shadow.mapSize.height = 2048;
scene.add(dirLight);

const fillLight = new THREE.DirectionalLight(0xbadfff, 0.8);
fillLight.position.set(-10, 10, -10);
scene.add(fillLight);

// --- State ---
const cellMeshes = []; // Will store references to cell meshes
let highlightedCellId = null;
let loadedModel = null;

// --- Test Geometry (To verify renderer) ---
const testCube = new THREE.Mesh(
  new THREE.BoxGeometry(2, 2, 2),
  new THREE.MeshStandardMaterial({ color: 0x00ff00, wireframe: true })
);
testCube.position.set(0, 5, 0);
scene.add(testCube);
console.log("Test cube added to scene at (0,5,0)");

// --- Load Model ---
console.log("Starting FBX load...");
const loader = new FBXLoader();
loader.load(
  MODEL_PATH,
  (object) => {
    loadedModel = object;

    // Center the model
    const box = new THREE.Box3().setFromObject(object);
    const center = box.getCenter(new THREE.Vector3());
    object.position.sub(center); // Center at 0,0,0

    // Traverse and setup materials/cells
    let cellIndex = 1;
    object.traverse((child) => {
      if (child.isMesh) {
        child.castShadow = true;
        child.receiveShadow = true;

        // Apply a nice standard material if it doesn't have one or to unify the look
        // For now, we keep original materials but ensure they react to light
        if (child.material) {
          child.material.roughness = 0.4;
          child.material.metalness = 0.6;
          child.material.needsUpdate = true;
        }

        // Identify battery cells
        // We try to match by name. If the user didn't name them "Cell", we might need to adjust this.
        if (CELL_NAME_PATTERN.test(child.name)) {
          console.log("Found cell:", child.name);

          // Store reference for data updates
          cellMeshes.push({
            id: cellIndex++,
            mesh: child,
            originalMaterial: child.material.clone(),
            baseColor: child.material.color.clone()
          });
        }
      }
    });

    scene.add(object);

    // Fit camera to object
    const size = box.getSize(new THREE.Vector3()).length();
    const fitOffset = 1.4; // Zoom out to show whole model
    const maxSize = size * fitOffset;
    const fitHeightDistance = maxSize / (2 * Math.atan((Math.PI * camera.fov) / 360));
    const fitWidthDistance = fitHeightDistance / camera.aspect;
    const distance = Math.max(fitHeightDistance, fitWidthDistance);

    const direction = controls.target.clone().sub(camera.position).normalize().multiplyScalar(distance);
    controls.maxDistance = distance * 3;
    controls.target.copy(new THREE.Vector3(0, 0, 0));

    camera.near = distance / 100;
    camera.far = distance * 100;
    camera.updateProjectionMatrix();

    camera.position.copy(controls.target).sub(direction);
    controls.update();

    console.log(`Loaded model with ${cellMeshes.length} detected cells.`);

    // If no cells found, maybe log all names to help debug
    if (cellMeshes.length === 0) {
      console.warn("No cells found matching pattern. Logging all mesh names:");
      object.traverse(c => { if (c.isMesh) console.log(c.name); });

      // Fallback: Add a placeholder box so the user sees SOMETHING
      const placeholder = new THREE.Mesh(
        new THREE.BoxGeometry(5, 5, 5),
        new THREE.MeshStandardMaterial({ color: 0xff0000, wireframe: true })
      );
      scene.add(placeholder);

      // Add a text label if possible, or just log
      const msg = document.createElement('div');
      msg.style.position = 'absolute';
      msg.style.bottom = '20px';
      msg.style.left = '50%';
      msg.style.transform = 'translateX(-50%)';
      msg.style.color = 'orange';
      msg.innerText = "Warning: No battery cells found in model. Showing wireframe box.";
      document.body.appendChild(msg);
    }
  },
  (xhr) => {
    console.log((xhr.loaded / xhr.total) * 100 + "% loaded");
  },
  (error) => {
    console.error("An error happened loading the FBX:", error);
    // Create an on-screen error message for the user
    const errorDiv = document.createElement('div');
    errorDiv.style.position = 'absolute';
    errorDiv.style.top = '50%';
    errorDiv.style.left = '50%';
    errorDiv.style.transform = 'translate(-50%, -50%)';
    errorDiv.style.background = 'rgba(255, 0, 0, 0.8)';
    errorDiv.style.color = 'white';
    errorDiv.style.padding = '20px';
    errorDiv.style.borderRadius = '8px';
    errorDiv.style.zIndex = '1000';
    errorDiv.innerHTML = `
      <h3>3D Model Loading Error</h3>
      <p>${error.message || 'Unknown error'}</p>
      <p style="font-size: 0.8em; margin-top: 10px">
        Note: If you are opening index.html directly (file://), 
        browsers block 3D models for security. 
        <br>Try using a local server (e.g., VS Code Live Server).
      </p>
    `;
    document.body.appendChild(errorDiv);
  }
);

// --- Interaction (Raycaster) ---
const raycaster = new THREE.Raycaster();
const mouse = new THREE.Vector2();

window.addEventListener('click', onMouseClick, false);

function onMouseClick(event) {
  // Calculate mouse position in normalized device coordinates
  mouse.x = (event.clientX / window.innerWidth) * 2 - 1;
  mouse.y = -(event.clientY / window.innerHeight) * 2 + 1;

  raycaster.setFromCamera(mouse, camera);

  // Intersect against the loaded model
  if (loadedModel) {
    const intersects = raycaster.intersectObjects(loadedModel.children, true);

    if (intersects.length > 0) {
      const hitObject = intersects[0].object;
      // Find if this object corresponds to a known cell
      const cellEntry = cellMeshes.find(entry => entry.mesh === hitObject);

      if (cellEntry) {
        console.log("Clicked cell ID:", cellEntry.id);

        const isCurrentlySelected = highlightedCellId === cellEntry.id && detailPanel.classList.contains("is-visible");

        if (isCurrentlySelected) {
          // Toggle off
          detailPanel.classList.remove("is-visible");
          highlightCell(null);
        } else {
          // Show detail
          showDetail(cellEntry.id);
          highlightCell(cellEntry.id);
        }
      } else {
        // Clicked something else (frame, etc)
        highlightCell(null);
        document.querySelector("[data-detail-panel]").classList.remove("is-visible");
      }
    }
  }
}


// --- UI & Data Logic ---
const packVoltageEl = document.querySelector("[data-pack-voltage]");
const packTempEl = document.querySelector("[data-pack-temp]");
const fanSpeed1El = document.querySelector("[data-fan-speed-1]");
const fanSpeed2El = document.querySelector("[data-fan-speed-2]");
const thermalTrendEl = document.querySelector("[data-thermal-trend]");
const cellGridEl = document.querySelector(".cell-grid");
const detailPanel = document.querySelector("[data-detail-panel]");
const detailTitle = document.querySelector("[data-cell-title]");
const detailVoltage = document.querySelector("[data-cell-voltage]");
const detailTemp = document.querySelector("[data-cell-temperature]");
const detailDelta = document.querySelector("[data-cell-delta]");
const closePanelBtn = document.querySelector("[data-close-panel]");
const modeButtons = document.querySelectorAll(".mode-btn");
const dataPulseEl = document.getElementById("data-pulse");

let currentState = null;

// Mock State - Will be replaced by real data stream later
const state = {
  cells: Array.from({ length: 10 }, (_, i) => ({
    id: i + 1,
    voltage: 3.8 + Math.random() * 0.4,
    temperature: 28 + Math.random() * 6,
  })),
  mode: "Balanced",
  fan1: { rpm: 1200 },
  fan2: { rpm: 1200 },
};

function populateCellGrid() {
  const fragment = document.createDocumentFragment();
  state.cells.forEach((cell) => {
    const card = document.createElement("div");
    card.className = "cell-card";
    // Battery Icon HTML
    card.innerHTML = `
      <div class="battery-icon">
        <div class="battery-level" style="height: 0%"></div>
      </div>
      <div class="cell-info">
        <p class="cell-card__title">Cell ${cell.id}</p>
        <p class="cell-card__value">-- V</p>
      </div>
    `;
    card.dataset.cellId = cell.id;
    card.addEventListener("click", () => {
      const isCurrentlySelected = highlightedCellId === cell.id && detailPanel.classList.contains("is-visible");

      if (isCurrentlySelected) {
        // Toggle off
        detailPanel.classList.remove("is-visible");
        highlightCell(null);
      } else {
        // Show detail
        showDetail(cell.id);
        highlightCell(cell.id);
      }
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

  // Reset all cells first
  cellMeshes.forEach(entry => {
    // If we want to reset to original color, we can. 
    // But animateCells is running constantly, so we should just set a 'highlight' flag or scale
    const isSelected = entry.id === cellId;

    gsap.to(entry.mesh.scale, {
      x: isSelected ? 1.1 : 1,
      y: isSelected ? 1.1 : 1,
      z: isSelected ? 1.1 : 1,
      duration: 0.4,
      ease: "back.out(1.7)"
    });

    if (isSelected) {
      entry.mesh.material.emissive.setHex(0x0a84ff);
      entry.mesh.material.emissiveIntensity = 0.5;
    } else {
      // Let animateCells handle the rest
      entry.mesh.material.emissive.setHex(0x000000);
      entry.mesh.material.emissiveIntensity = 0;
    }
  });
}

// Trigger a data pulse based on transmission rate
function triggerDataPulse(dataRate) {
  if (!dataPulseEl) return;

  // Map data rate (using fan RPM as proxy: 800-1800 RPM) to pulse speed
  // Higher data rate = faster pulse
  const minRate = 800;
  const maxRate = 1800;
  const minDuration = 0.8; // Fast pulse (0.8s) for high data rate
  const maxDuration = 2.5; // Slow pulse (2.5s) for low data rate

  const normalizedRate = Math.min(Math.max((dataRate - minRate) / (maxRate - minRate), 0), 1);
  const pulseDuration = maxDuration - (normalizedRate * (maxDuration - minDuration));

  // Set the pulse duration as CSS variable
  dataPulseEl.style.setProperty('--pulse-duration', `${pulseDuration}s`);

  // Remove and re-add the active class to restart animation
  dataPulseEl.classList.remove('active');

  // Force reflow to restart animation
  void dataPulseEl.offsetWidth;

  dataPulseEl.classList.add('active');
}

function updateHud(data) {
  const packVoltage = data.cells.reduce((acc, cell) => acc + cell.voltage, 0);
  const peakTemp = Math.max(...data.cells.map((cell) => cell.temperature));
  packVoltageEl.textContent = `${packVoltage.toFixed(1)} V`;
  packTempEl.textContent = `${peakTemp.toFixed(1)} °C`;

  // Update both fan speeds with formatting
  fanSpeed1El.textContent = `${Math.round(data.fan1.rpm).toLocaleString()} RPM`;
  fanSpeed2El.textContent = `${Math.round(data.fan2.rpm).toLocaleString()} RPM`;


  // Update data streaming status
  const streamMessages = [
    "Streaming data",
    "Transmitting packets",
    "Live telemetry",
    "Data flowing"
  ];
  const randomMessage = streamMessages[Math.floor(Date.now() / 3000) % streamMessages.length];
  thermalTrendEl.textContent = randomMessage;

  // Trigger data pulse with speed based on data rate
  // Use average fan speed as a proxy for data transmission rate
  const avgFanSpeed = (data.fan1.rpm + data.fan2.rpm) / 2;
  triggerDataPulse(avgFanSpeed);


  document.querySelectorAll(".cell-card").forEach((card) => {
    const id = Number(card.dataset.cellId);
    const cell = data.cells.find((c) => c.id === id);
    if (!cell) return;

    // Update text
    card.querySelector(".cell-card__value").textContent = `${cell.voltage.toFixed(2)} V`;

    // Update Battery Icon
    const levelEl = card.querySelector(".battery-level");
    if (levelEl) {
      // Map 3.2V (0%) to 4.2V (100%)
      const pct = Math.max(0, Math.min(100, ((cell.voltage - 3.2) / (4.2 - 3.2)) * 100));
      levelEl.style.height = `${pct}%`;

      // Color based on percentage
      if (pct < 20) levelEl.style.backgroundColor = 'var(--danger-color)';
      else if (pct < 50) levelEl.style.backgroundColor = 'orange';
      else levelEl.style.backgroundColor = 'var(--success-color)';
    }
  });
}

function colorForVoltage(voltage) {
  // Map voltage 3.2-4.2V to a color gradient (Red -> Green -> Blue)
  // Simple HSL: 0 (Red) to 120 (Green)
  const normalized = THREE.MathUtils.clamp((voltage - 3.2) / (4.2 - 3.2), 0, 1);
  const hue = THREE.MathUtils.lerp(0, 0.33, normalized); // 0 to 120 degrees
  const color = new THREE.Color();
  color.setHSL(hue, 0.8, 0.5);
  return color;
}

function animateCells(data) {
  data.cells.forEach((cellData) => {
    // Find corresponding mesh
    // Note: If we have more data cells than meshes, some won't show.
    // If we have more meshes than data, we loop through available meshes.
    // Here we assume ID matches index or we map sequentially if IDs don't match.

    const meshEntry = cellMeshes.find((entry) => entry.id === cellData.id);
    if (!meshEntry) return;

    // Don't override highlight color
    if (highlightedCellId === meshEntry.id) return;

    const voltageColor = colorForVoltage(cellData.voltage);

    // Smoothly transition color
    gsap.to(meshEntry.mesh.material.color, {
      r: voltageColor.r,
      g: voltageColor.g,
      b: voltageColor.b,
      duration: 0.6,
      ease: "power2.out",
    });
  });
}

// --- Animation Loop ---
const clock = new THREE.Clock();

function tick() {
  const delta = clock.getDelta();
  controls.update();

  // Gentle rotation of the whole model
  if (loadedModel) {
    loadedModel.rotation.y += delta * 0.1; // Slow rotation
  }

  renderer.render(scene, camera);
  requestAnimationFrame(tick);
}

tick();

// --- Power Mode Switching ---
modeButtons.forEach(btn => {
  btn.addEventListener("click", function () {
    // Remove active class from all buttons
    modeButtons.forEach(b => b.classList.remove("active"));

    // Add active class to clicked button
    this.classList.add("active");

    // Update the state mode
    state.mode = this.dataset.mode;

    console.log(`Power mode changed to: ${state.mode}`);
  });
});

// --- Mock Data Stream ---
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

  // Fan speed varies based on power mode
  let modeMultiplier = 1.0;
  if (state.mode === "Low-Power") {
    modeMultiplier = 0.7; // 70% speed
  } else if (state.mode === "High-Power") {
    modeMultiplier = 1.4; // 140% speed
  }

  const baseFan1 =
    1200 +
    Math.sin(Date.now() * 0.0012) * 220 +
    THREE.MathUtils.randFloatSpread(60);

  const baseFan2 =
    1200 +
    Math.sin(Date.now() * 0.0010) * 200 + // Slightly different phase
    THREE.MathUtils.randFloatSpread(60);

  const fan1rpm = baseFan1 * modeMultiplier;
  const fan2rpm = baseFan2 * modeMultiplier;

  currentState = {
    ...state,
    cells: nextCells,
    fan1: { rpm: fan1rpm },
    fan2: { rpm: fan2rpm },
  };

  state.cells = nextCells;

  updateHud(currentState);
  animateCells(currentState);
}

// Initialize
populateCellGrid();
mockStream();
setInterval(mockStream, 1500);

// Resize Handler
// --- 3D Tilt Effect for UI Panels ---
document.querySelectorAll(".glass-panel").forEach((panel) => {
  panel.addEventListener("mousemove", (e) => {
    const rect = panel.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;

    const centerX = rect.width / 2;
    const centerY = rect.height / 2;

    const rotateX = ((y - centerY) / centerY) * -0.5; // Max rotation deg (barely noticeable)
    const rotateY = ((x - centerX) / centerX) * 0.5;

    panel.style.transform = `perspective(1000px) rotateX(${rotateX}deg) rotateY(${rotateY}deg) scale(1.005)`;
  });

  panel.addEventListener("mouseleave", () => {
    panel.style.transform = "perspective(1000px) rotateX(0) rotateY(0) scale(1)";
  });
});

