import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";
import { FBXLoader } from "three/addons/loaders/FBXLoader.js";
import { GLTFLoader } from "three/addons/loaders/GLTFLoader.js";

const { gsap } = window;

// --- Configuration ---
const MODEL_PATH = "battery_design.fbx";
const CELL_NAME_PATTERN = /\b(cell|cyl|cylinder)\b/i; // Generic fallback
const EXPLICIT_CELL_NAME_PATTERN = /\bbattery\s*cell\b|\bcell[\s_-]*\d+\b/i;
const SHELL_NAME_PATTERN = /shell|case|housing|enclosure|cover|body|chassis/i;
const FAN_NAME_PATTERN = /fan|blade|impeller|rotor/i;
const FAN_BLADE_NAME_PATTERN = /blade|impeller|rotor/i;
const EXPLICIT_FAN_BLADE_NAME_PATTERN = /\bfan[\s_-]*blade\b/i;
const FAN_SHELL_NAME_PATTERN = /\bfan[\s_-]*shell\b|\bfanshell\b|\bfan[\s_-]*frame\b|\bfan[\s_-]*housing\b/i;
const PCB_NAME_PATTERN = /pcb|board|mainboard|motherboard|controller|logic/i;
const HARDWARE_NAME_PATTERN = /screw|bolt|nut|washer|standoff|header|connector|terminal|capacitor|resistor|inductor|mosfet|diode|ic|chip|wire|pin/i;
const AUTO_ROTATE_MODEL = false;
const FAST_MODEL_INIT = true;
const SHELL_CONNECTED_OPACITY = 0.2;
let CONNECTION_TRANSITION_MS = 1600;
const CONNECTED_MODEL_POSITION_OFFSET = new THREE.Vector3(0.0, 0.0, 0.0);
const CONNECTED_MODEL_ROTATION_OFFSET = new THREE.Euler(-0.31737, 1.22493, 0.22785, "XYZ");
const FAN_SPIN_BASE_RAD_PER_SEC = 2.0;
const FAN_SPIN_MAX_RAD_PER_SEC = 28.0;
const FAN_TARGET_COUNT = 2;
const SHELL_TARGET_MAX = 8;
const BOARD_PLANE_MARGIN_RATIO = 0.03;
const BOARD_PLANE_MARGIN_MIN = 0.02;

// --- E-Load Configuration ---
const ELOAD_MODEL_PATH = "E-Load.glb";
const ELOAD_POSITION_OFFSET = new THREE.Vector3(25, 0, 0);  // Side-by-side
const ELOAD_SCALE = 1.0;  // Adjust after visual inspection
const ELOAD_LID_NAME_PATTERN = /lid|top|cover|cap/i;
const ELOAD_SHELL_NAME_PATTERN = /shell|case|housing|enclosure|body|chassis/i;
const ELOAD_FAN_BLADE_NAME_PATTERN = /fan[\s_-]*blade|blade|impeller|rotor|fan_blade/i;
const ELOAD_HEATSINK_NAME_PATTERN = /heatsink|heat[\s_-]*sink|fin|radiator|mosfet|fet|transistor|to[\s_-]*220|to[\s_-]*247|power[\s_-]*stage/i;
const ELOAD_FET_NAME_PATTERN = /mosfet|fet|transistor|to[\s_-]*220|to[\s_-]*247|power[\s_-]*stage/i;
const ELOAD_FAN_SPIN_BASE = 2.0;
const ELOAD_FAN_SPIN_MAX = 28.0;

const BOOT_STAGE_WEIGHTS = {
  bootstrap: 0.10,
  modelDownload: 0.65,
  modelProcess: 0.20,
  finalize: 0.05,
};
const BOOT_REVEAL_HOLD_MS = 450;
const BOOT_REVEAL_DURATION_MS = 1600;

const bootLoaderEl = document.getElementById("boot-loader");
const bootProgressFillEl = document.getElementById("boot-progress-fill");
const bootPercentEl = document.getElementById("boot-percent");
const bootStageEl = document.getElementById("boot-stage");
const bootDetailEl = document.getElementById("boot-detail");
const chromeMaskEl = document.getElementById("chrome-mask");
const chromeMaskBarEl = document.querySelector(".chrome-mask__bar");

const bootState = {
  stage: "bootstrap",
  stageProgress: {
    bootstrap: 0,
    modelDownload: 0,
    modelProcess: 0,
    finalize: 0,
  },
  detail: "Waiting to start...",
  uiReady: false,
  modelReady: false,
  bytesLoaded: 0,
  bytesTotal: 0,
  hidden: false,
  errored: false,
  hideStarted: false,
  handoffProgress: 0,
  chromeCueSent: false,
  phase: "loading",
};
let bootRevealHoldTimer = 0;
let startupConnectionTarget = false;

function clamp01(value) {
  return Math.max(0, Math.min(1, Number(value) || 0));
}

function waitForNextFrame() {
  return new Promise((resolve) => {
    window.requestAnimationFrame(() => resolve());
  });
}

function formatBootBytes(bytes) {
  const size = Number(bytes) || 0;
  if (size <= 0) return "0 B";
  const units = ["B", "KB", "MB", "GB"];
  const exponent = Math.min(Math.floor(Math.log(size) / Math.log(1024)), units.length - 1);
  const scaled = size / (1024 ** exponent);
  if (exponent === 0) return `${Math.round(scaled)} ${units[exponent]}`;
  return `${scaled.toFixed(1)} ${units[exponent]}`;
}

function computeBootPercent() {
  return (
    clamp01(bootState.stageProgress.bootstrap) * BOOT_STAGE_WEIGHTS.bootstrap +
    clamp01(bootState.stageProgress.modelDownload) * BOOT_STAGE_WEIGHTS.modelDownload +
    clamp01(bootState.stageProgress.modelProcess) * BOOT_STAGE_WEIGHTS.modelProcess +
    clamp01(bootState.stageProgress.finalize) * BOOT_STAGE_WEIGHTS.finalize
  ) * 100;
}

function setBootStage(stage, message) {
  bootState.stage = stage;
  if (bootStageEl && typeof message === "string") {
    bootStageEl.textContent = message;
  }
}

function setBootDetail(detail) {
  bootState.detail = detail;
  if (bootDetailEl && typeof detail === "string") {
    bootDetailEl.textContent = detail;
  }
}

function setBootStageProgress(stage, progress) {
  if (!(stage in bootState.stageProgress)) return;
  bootState.stageProgress[stage] = clamp01(progress);
  const percent = computeBootPercent();
  if (bootProgressFillEl) {
    bootProgressFillEl.style.width = `${percent.toFixed(2)}%`;
  }
  if (bootPercentEl) {
    bootPercentEl.textContent = `${Math.round(percent)}%`;
  }
}

function setModelProcessProgress(baseOffset, span, progressInPhase) {
  setBootStageProgress("modelProcess", baseOffset + (span * clamp01(progressInPhase)));
}

function updateBootDownloadProgress(loadedBytes, totalBytes) {
  const loaded = Math.max(0, Number(loadedBytes) || 0);
  const total = Number.isFinite(totalBytes) && totalBytes > 0 ? Number(totalBytes) : 0;
  bootState.bytesLoaded = loaded;
  bootState.bytesTotal = total;

  if (total > 0) {
    const ratio = loaded / total;
    setBootStageProgress("modelDownload", ratio);
    setBootDetail(`${formatBootBytes(loaded)} / ${formatBootBytes(total)}`);
    return;
  }

  setBootDetail(`${formatBootBytes(loaded)} downloaded`);
}

function maybeFinishBootLoader() {
  if (bootState.errored || bootState.hideStarted || bootState.hidden) return;
  if (!(bootState.uiReady && bootState.modelReady)) return;

  bootState.hideStarted = true;
  bootState.phase = "loading";
  setBootStage("finalize", "Finalizing startup...");
  setBootDetail("Preparing model handoff...");
  setBootStageProgress("finalize", 1);

  if (bootRevealHoldTimer) {
    window.clearTimeout(bootRevealHoldTimer);
    bootRevealHoldTimer = 0;
  }
  bootRevealHoldTimer = window.setTimeout(() => {
    bootRevealHoldTimer = 0;
    runRevealSequence();
  }, BOOT_REVEAL_HOLD_MS);
}

function runRevealSequence() {
  const loaderContent = document.getElementById("boot-loader-content");
  const loaderBarEl = document.querySelector(".boot-loader__bar");
  const loaderMetaEl = document.querySelector(".boot-loader__meta");
  const sceneCanvas = document.getElementById("scene");
  const hudRoot = document.querySelector(".hud");
  const detailPanel = document.querySelector(".detail-panel");
  const header = document.querySelector(".hud__header");

  const finishImmediately = () => {
    const startupBlend = startupConnectionTarget ? 1 : 0;
    connectionVisualProgress = startupBlend;
    connectionTransitionFrom = startupBlend;
    connectionTransitionTo = startupBlend;
    connectionTransitionActive = false;
    applyShellTransparency(startupBlend);
    applyModelConnectionPose(startupBlend);

    bootState.chromeCueSent = true;
    bootState.handoffProgress = 1;
    bootState.hidden = true;
    bootState.phase = "complete";
    document.body.classList.add("boot-chrome-cue");
    document.body.classList.remove("is-booting", "is-revealing");
    if (bootLoaderEl) bootLoaderEl.remove();
    if (hudRoot) {
      hudRoot.style.visibility = "visible";
      hudRoot.style.opacity = "1";
    }
    if (detailPanel) {
      detailPanel.style.visibility = "visible";
      detailPanel.style.opacity = "1";
    }
    if (sceneCanvas) {
      sceneCanvas.style.visibility = "visible";
      sceneCanvas.style.opacity = "1";
    }
    if (chromeMaskBarEl) {
      chromeMaskBarEl.style.opacity = "0";
      chromeMaskBarEl.style.transform = "translateY(-18px)";
    }
  };

  if (!bootLoaderEl || !gsap) {
    finishImmediately();
    return;
  }

  const leftPanels = gsap.utils.toArray(".left-column .glass-panel");
  const rightPanels = gsap.utils.toArray(".right-column .glass-panel");
  const statusEl = document.querySelector(".status");

  bootState.phase = "handoff";
  bootState.handoffProgress = 0;
  bootState.chromeCueSent = false;
  document.body.classList.add("is-revealing");
  document.body.classList.remove("is-booting");

  const revealDuration = BOOT_REVEAL_DURATION_MS / 1000;
  const modelTargetBlend = startupConnectionTarget ? 1 : 0;
  const cameraEnd = camera.position.clone();
  const targetEnd = controls.target.clone();
  const cameraStart = cameraEnd.clone().add(new THREE.Vector3(1.6, 1.2, 2.9));
  const targetStart = targetEnd.clone().add(new THREE.Vector3(-0.35, 0.16, 0.12));
  camera.position.copy(cameraStart);
  controls.target.copy(targetStart);
  controls.update();

  const modelScaleTo = loadedModel
    ? { x: loadedModel.scale.x, y: loadedModel.scale.y, z: loadedModel.scale.z }
    : null;
  const modelScaleFrom = modelScaleTo
    ? { x: modelScaleTo.x * 0.92, y: modelScaleTo.y * 0.92, z: modelScaleTo.z * 0.92 }
    : null;

  const allAnimTargets = [
    loaderContent, bootStageEl, loaderBarEl, loaderMetaEl,
    hudRoot, detailPanel, header, statusEl,
    chromeMaskEl, chromeMaskBarEl, ...leftPanels, ...rightPanels,
  ].filter(Boolean);
  gsap.set(allAnimTargets, { willChange: "transform, opacity" });

  if (sceneCanvas) gsap.set(sceneCanvas, { visibility: "visible", opacity: 1 });
  if (hudRoot) gsap.set(hudRoot, { visibility: "visible", opacity: 0 });
  if (detailPanel) gsap.set(detailPanel, { visibility: "visible", opacity: 0 });
  if (chromeMaskBarEl) gsap.set(chromeMaskBarEl, { opacity: 1, y: 0 });

  const cameraBlend = { t: 0 };
  const connectionBlend = { t: connectionVisualProgress };

  const tl = gsap.timeline({
    defaults: { ease: "power3.out" },
    onUpdate: () => {
      const progress = Number(tl.progress().toFixed(3));
      bootState.handoffProgress = progress;
      if (!bootState.chromeCueSent && progress >= 0.52) {
        bootState.chromeCueSent = true;
        bootState.phase = "chrome_cue";
        document.body.classList.add("boot-chrome-cue");
      }
    },
    onComplete: () => {
      bootState.hidden = true;
      bootState.phase = "complete";
      bootState.handoffProgress = 1;
      document.body.classList.remove("is-revealing");
      gsap.set(allAnimTargets, { willChange: "auto", clearProps: "willChange" });
      if (bootLoaderEl) bootLoaderEl.remove();
      gsap.set(
        [sceneCanvas, hudRoot, detailPanel, header, statusEl, ...leftPanels, ...rightPanels].filter(Boolean),
        { clearProps: "all" },
      );
      if (chromeMaskBarEl) {
        gsap.set(chromeMaskBarEl, { opacity: 0, y: -18 });
      }
    },
  });

  tl.to(
    cameraBlend,
    {
      t: 1,
      duration: revealDuration * 0.35,
      ease: "expo.out",
      onUpdate: () => {
        camera.position.lerpVectors(cameraStart, cameraEnd, cameraBlend.t);
        controls.target.lerpVectors(targetStart, targetEnd, cameraBlend.t);
        controls.update();
      },
    },
    0,
  );

  if (loadedModel && modelScaleFrom && modelScaleTo) {
    tl.fromTo(
      loadedModel.scale,
      modelScaleFrom,
      {
        ...modelScaleTo,
        duration: revealDuration * 0.32,
        ease: "back.out(1.25)",
      },
      revealDuration * 0.08,
    );
  }

  tl.to(
    connectionBlend,
    {
      t: modelTargetBlend,
      duration: revealDuration * 0.60,
      ease: "sine.inOut",
      onStart: () => {
        connectionTransitionActive = false;
      },
      onUpdate: () => {
        connectionVisualProgress = connectionBlend.t;
        applyShellTransparency(connectionVisualProgress);
        applyModelConnectionPose(connectionVisualProgress);
      },
    },
    revealDuration * 0.15,
  );

  tl.to(
    chromeMaskBarEl,
    {
      opacity: 0,
      y: -18,
      duration: revealDuration * 0.32,
      ease: "power2.out",
    },
    revealDuration * 0.52,
  );

  tl.to(
    loaderContent,
    {
      opacity: 0,
      y: 10,
      duration: revealDuration * 0.22,
      ease: "power2.in",
    },
    revealDuration * 0.78,
  );

  tl.to(
    hudRoot,
    { opacity: 1, duration: revealDuration * 0.24, ease: "power2.out" },
    revealDuration * 0.45,
  );

  if (header) {
    tl.fromTo(
      header,
      { opacity: 0, y: -16 },
      { opacity: 1, y: 0, duration: revealDuration * 0.26, ease: "power3.out" },
      revealDuration * 0.49,
    );
  }

  tl.fromTo(
    leftPanels,
    { opacity: 0, x: -24 },
    {
      opacity: 1,
      x: 0,
      duration: revealDuration * 0.30,
      stagger: revealDuration * 0.03,
      ease: "power3.out",
    },
    revealDuration * 0.54,
  );

  tl.fromTo(
    rightPanels,
    { opacity: 0, x: 24 },
    {
      opacity: 1,
      x: 0,
      duration: revealDuration * 0.30,
      stagger: revealDuration * 0.03,
      ease: "power3.out",
    },
    revealDuration * 0.58,
  );

  if (statusEl) {
    tl.fromTo(
      statusEl,
      { opacity: 0, y: 10 },
      { opacity: 1, y: 0, duration: revealDuration * 0.22, ease: "power2.out" },
      revealDuration * 0.70,
    );
  }
}

function markBootUiReady() {
  if (bootState.uiReady) return;
  bootState.uiReady = true;
  setBootStageProgress("bootstrap", 1);
  maybeFinishBootLoader();
}

function markBootModelReady() {
  if (bootState.modelReady || bootState.errored) return;
  window.requestAnimationFrame(() => {
    window.requestAnimationFrame(() => {
      if (bootState.modelReady || bootState.errored) return;
      bootState.modelReady = true;
      maybeFinishBootLoader();
    });
  });
}

function setBootError(message, error = null) {
  if (bootRevealHoldTimer) {
    window.clearTimeout(bootRevealHoldTimer);
    bootRevealHoldTimer = 0;
  }
  bootState.errored = true;
  bootState.handoffProgress = 0;
  bootState.phase = "error";
  if (bootLoaderEl) {
    bootLoaderEl.classList.remove("is-hidden");
    bootLoaderEl.classList.add("is-error");
  }
  document.body.classList.remove("is-revealing");
  setBootStage("finalize", message || "Startup failed.");
  setBootDetail("See console for details.");
  if (error) {
    console.error("[BMS] Startup error:", error);
  }
}

window.__bmsSetStartupConnectionTarget = function __bmsSetStartupConnectionTarget(connected) {
  startupConnectionTarget = Boolean(connected);
  return {
    ok: true,
    target: startupConnectionTarget,
  };
};

window.__bmsBootDebug = function __bmsBootDebug() {
  return {
    stage: bootState.stage,
    percent: Number(computeBootPercent().toFixed(2)),
    bytesLoaded: bootState.bytesLoaded,
    bytesTotal: bootState.bytesTotal,
    uiReady: bootState.uiReady,
    modelReady: bootState.modelReady,
    hidden: bootState.hidden,
    errored: bootState.errored,
    phase: bootState.phase,
    handoffProgress: Number((bootState.handoffProgress || 0).toFixed(3)),
  };
};

setBootStage("bootstrap", "Initializing dashboard...");
setBootDetail("Preparing renderer...");
setBootStageProgress("bootstrap", 0.1);

// --- Scene Setup ---
const canvas = document.getElementById("scene");
const renderer = new THREE.WebGLRenderer({
  canvas,
  antialias: true,
  alpha: true,
  powerPreference: "high-performance",
});
renderer.shadowMap.enabled = false;
renderer.shadowMap.type = THREE.PCFSoftShadowMap;
renderer.setPixelRatio(Math.min(window.devicePixelRatio, 1.5));
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
const DEFAULT_CAMERA_POSITION = new THREE.Vector3(12, 10, 20);  // BMS-only view
const DEFAULT_CAMERA_TARGET = new THREE.Vector3(0, 0, 0);  // Centered on BMS model
camera.position.copy(DEFAULT_CAMERA_POSITION);
camera.up.set(0, 1, 0);

const controls = new OrbitControls(camera, renderer.domElement);
controls.enableDamping = true;
controls.dampingFactor = 0.05;
controls.enablePan = false;
controls.maxPolarAngle = Math.PI / 2.1;
controls.target.copy(DEFAULT_CAMERA_TARGET);

// --- E-Load Scene Setup (LAZY — initialized on first tab switch) ---
const eloadCanvas = document.getElementById("eload-scene");
let eloadRenderer = null;
let eloadScene = null;
let eloadCamera = null;
let eloadControls = null;
let activePageId = "bms";
let eloadSceneInitialized = false;

function initEloadScene() {
  if (eloadSceneInitialized || !eloadCanvas) return;
  eloadSceneInitialized = true;

  eloadRenderer = new THREE.WebGLRenderer({
    canvas: eloadCanvas,
    antialias: true,
    alpha: true,
    powerPreference: "high-performance",
  });
  eloadRenderer.shadowMap.enabled = false;
  eloadRenderer.setPixelRatio(Math.min(window.devicePixelRatio, 1.5));
  eloadRenderer.setSize(window.innerWidth, window.innerHeight);
  eloadRenderer.toneMapping = THREE.ACESFilmicToneMapping;
  eloadRenderer.toneMappingExposure = 1.2;

  eloadScene = new THREE.Scene();

  // Set up environment map for reflections on the scene itself
  eloadEnvMap = buildStudioEnvMap();
  eloadScene.environment = eloadEnvMap;

  eloadCamera = new THREE.PerspectiveCamera(
    45,
    window.innerWidth / window.innerHeight,
    0.1,
    50000
  );
  eloadCamera.position.set(12, 10, 20);
  eloadCamera.up.set(0, 1, 0);

  eloadControls = new OrbitControls(eloadCamera, eloadCanvas);
  eloadControls.enableDamping = true;
  eloadControls.dampingFactor = 0.05;
  eloadControls.enablePan = false;
  eloadControls.maxPolarAngle = Math.PI / 2.1;
  eloadControls.maxDistance = 20000;
  eloadControls.target.set(0, 0, 0);

  // E-Load scene lighting (matching BMS aesthetics)
  const eloadAmbient = new THREE.AmbientLight(0xffffff, 0.4);
  eloadScene.add(eloadAmbient);

  const eloadDirLight = new THREE.DirectionalLight(0xffffff, 1.5);
  eloadDirLight.position.set(10, 20, 10);
  eloadScene.add(eloadDirLight);

  const eloadFillLight = new THREE.DirectionalLight(0xbadfff, 0.8);
  eloadFillLight.position.set(-10, 10, -10);
  eloadScene.add(eloadFillLight);

  // Load E-Load model now
  loadEloadModel()
    .then((eloadModel) => {
      if (eloadModel) return initializeEloadModel(eloadModel);
    })
    .catch((error) => {
      console.warn('[BMS] E-Load initialization failed:', error);
    });

  console.log('[BMS] E-Load scene initialized (lazy)');
}

setBootStageProgress("bootstrap", 0.45);
setBootDetail("Configuring scene...");

// --- Lighting ---
const ambientLight = new THREE.AmbientLight(0xffffff, 0.4);
scene.add(ambientLight);

const dirLight = new THREE.DirectionalLight(0xffffff, 1.5);
dirLight.position.set(10, 20, 10);
dirLight.castShadow = false;
scene.add(dirLight);

const fillLight = new THREE.DirectionalLight(0xbadfff, 0.8);
fillLight.position.set(-10, 10, -10);
scene.add(fillLight);
setBootStageProgress("bootstrap", 0.7);
setBootDetail("Preparing interface...");

// --- State ---
const cellMeshes = []; // Will store references to cell meshes
const shellMeshes = [];
const fanMeshes = [];
const boardMeshes = [];       // PCB / mainboard meshes
const connectorMeshes = [];   // ports, connectors, terminals, headers, pins, wires
const meshInfos = [];
const selectedCellUuidSet = new Set();
let highlightedCellId = null;
let loadedModel = null;
let loadedEloadModel = null;
const eloadShellMeshes = [];
const eloadLidMeshes = [];
const eloadFanBladeMeshes = [];   // { mesh, spinNode, axis }
const eloadHeatsinkMeshes = [];   // meshes for heat visualization
const eloadBoardMeshes = [];      // PCB / mainboard meshes
const eloadConnectorMeshes = [];  // ports, connectors, terminals, headers
const eloadThermalEntries = [];   // { mesh, originalMat } for material swapping
let eloadThermalShaderMat = null; // shared ShaderMaterial for thermal viz
let eloadFetWorldPositions = [];  // Vector3[] — world-space centers of FET heat sources
let eloadFetHeatLevels = [0.5, 0.5, 0.5, 0.5]; // per-FET heat (0-1), telemetry-driven
let eloadThermalHeatRadius = 1.0; // world-space radius for heat falloff
let eloadFanSpinEnabled = true;
let eloadFanSpinSpeed = 0.5;      // 0..1 normalized
let eloadHeatVizEnabled = true;
let eloadHeatIntensity = 0.5;     // 0..1 normalized
let eloadEnvMap = null;
let eloadHeatClock = 0;           // accumulator for heat animation

// --- E-Load reveal transition state ---
const ELOAD_REVEAL_MS = 2200;     // duration of the smooth reveal
let eloadRevealProgress = 0;      // 0 = solid/default, 1 = revealed/transparent
let eloadRevealFrom = 0;
let eloadRevealTo = 0;
let eloadRevealStartMs = 0;
let eloadRevealActive = false;
let eloadHasRevealed = false;     // tracks if we've ever triggered a reveal
// Camera poses (set after model loads)
let eloadCameraDefaultPos = null; // hero exterior view
let eloadCameraDefaultTarget = null;
let eloadCameraRevealPos = null;  // reveal interior view
let eloadCameraRevealTarget = null;
let eloadCameraFromPos = null;    // transition "from" snapshot
let eloadCameraFromTarget = null;
let isBackendConnected = false;
let fanSpinRpm = 0;
let connectionVisualProgress = 0;
let connectionTransitionStartMs = 0;
let connectionTransitionFrom = 0;
let connectionTransitionTo = 0;
let connectionTransitionActive = false;
let modelDefaultTransform = null;
let modelConnectedTransform = null;
let viewResetTransitionActive = false;
let viewResetTransitionStartMs = 0;
const viewResetFromPosition = new THREE.Vector3();
const viewResetFromTarget = new THREE.Vector3();
const viewResetToPosition = new THREE.Vector3();
const viewResetToTarget = new THREE.Vector3();
let lastModelSelectionDebug = {
  boardPlaneY: null,
  boardPlaneMargin: null,
  meshInfoCount: 0,
  cellCandidateCount: 0,
  selectedCellCount: 0,
  shellCount: 0,
  fanCount: 0,
  selectedCells: [],
  thresholds: {
    roundnessMin: 0.82,
    roundnessMax: 1.22,
    elongationMin: 1.25,
    elongationMax: 4.2,
    minVolumeRatio: 0.001,
    maxVolumeRatio: 0.05,
  },
};

function easeInOutCubic(t) {
  if (t <= 0) return 0;
  if (t >= 1) return 1;
  return t < 0.5
    ? 4 * t * t * t
    : 1 - Math.pow(-2 * t + 2, 3) / 2;
}

function configureModelPoseTargets(model) {
  if (!model) return;
  modelDefaultTransform = {
    position: model.position.clone(),
    quaternion: model.quaternion.clone(),
  };

  const defaultBounds = new THREE.Box3().setFromObject(model);
  const defaultBoundsCenter = defaultBounds.isEmpty()
    ? modelDefaultTransform.position.clone()
    : defaultBounds.getCenter(new THREE.Vector3());

  const connectedQuaternionOffset = new THREE.Quaternion().setFromEuler(CONNECTED_MODEL_ROTATION_OFFSET);
  const connectedQuaternion = modelDefaultTransform.quaternion.clone().multiply(connectedQuaternionOffset);
  const connectedPositionBase = modelDefaultTransform.position.clone().add(CONNECTED_MODEL_POSITION_OFFSET);

  // Preserve visual centering: compute how bounds center shifts after connected rotation,
  // then compensate with an opposite translation.
  const restorePosition = model.position.clone();
  const restoreQuaternion = model.quaternion.clone();
  model.position.copy(connectedPositionBase);
  model.quaternion.copy(connectedQuaternion);
  model.updateMatrixWorld(true);
  const connectedBounds = new THREE.Box3().setFromObject(model);
  const connectedBoundsCenter = connectedBounds.isEmpty()
    ? connectedPositionBase.clone()
    : connectedBounds.getCenter(new THREE.Vector3());

  model.position.copy(restorePosition);
  model.quaternion.copy(restoreQuaternion);
  model.updateMatrixWorld(true);

  const centeringCompensation = defaultBoundsCenter.clone().sub(connectedBoundsCenter);
  modelConnectedTransform = {
    position: connectedPositionBase.add(centeringCompensation),
    quaternion: connectedQuaternion,
  };
}

function applyModelConnectionPose(blend) {
  if (!loadedModel || !modelDefaultTransform || !modelConnectedTransform) return;
  const clampedBlend = THREE.MathUtils.clamp(blend, 0, 1);
  loadedModel.position.copy(modelDefaultTransform.position).lerp(modelConnectedTransform.position, clampedBlend);
  loadedModel.quaternion.copy(modelDefaultTransform.quaternion).slerp(modelConnectedTransform.quaternion, clampedBlend);
}

function startConnectionTransition(connected) {
  const target = connected ? 1 : 0;
  if (
    !connectionTransitionActive &&
    Math.abs(connectionVisualProgress - target) <= 1e-4 &&
    Math.abs(connectionTransitionTo - target) <= 1e-4
  ) {
    return;
  }

  connectionTransitionFrom = connectionVisualProgress;
  connectionTransitionTo = target;
  connectionTransitionStartMs = performance.now();
  connectionTransitionActive = true;
}

function startViewResetTransition() {
  viewResetFromPosition.copy(camera.position);
  viewResetFromTarget.copy(controls.target);
  viewResetToPosition.copy(DEFAULT_CAMERA_POSITION);
  viewResetToTarget.copy(DEFAULT_CAMERA_TARGET);

  // Ensure maxDistance allows the default camera position
  const defaultCameraDistance = DEFAULT_CAMERA_POSITION.length();
  controls.maxDistance = Math.max(controls.maxDistance || Infinity, defaultCameraDistance * 1.5);

  if (
    viewResetFromPosition.distanceToSquared(viewResetToPosition) < 1e-8 &&
    viewResetFromTarget.distanceToSquared(viewResetToTarget) < 1e-8
  ) {
    viewResetTransitionActive = false;
    camera.position.copy(DEFAULT_CAMERA_POSITION);
    controls.target.copy(DEFAULT_CAMERA_TARGET);
    camera.up.set(0, 1, 0);
    controls.update();
    return;
  }

  viewResetTransitionStartMs = performance.now();
  viewResetTransitionActive = true;
}

function cancelViewResetTransition() {
  viewResetTransitionActive = false;
}

function updateViewResetTransition(nowMs) {
  if (!viewResetTransitionActive) return;

  const elapsed = Math.max(0, nowMs - viewResetTransitionStartMs);
  const t = Math.min(1, elapsed / CONNECTION_TRANSITION_MS);
  const eased = easeInOutCubic(t);

  camera.position.copy(viewResetFromPosition).lerp(viewResetToPosition, eased);
  controls.target.copy(viewResetFromTarget).lerp(viewResetToTarget, eased);
  camera.up.set(0, 1, 0);

  if (t >= 1) {
    camera.position.copy(DEFAULT_CAMERA_POSITION);
    controls.target.copy(DEFAULT_CAMERA_TARGET);
    viewResetTransitionActive = false;
  }
}

function updateConnectionVisualState(nowMs) {
  if (connectionTransitionActive) {
    const elapsed = Math.max(0, nowMs - connectionTransitionStartMs);
    const t = Math.min(1, elapsed / CONNECTION_TRANSITION_MS);
    const eased = easeInOutCubic(t);
    connectionVisualProgress = THREE.MathUtils.lerp(connectionTransitionFrom, connectionTransitionTo, eased);
    if (t >= 1) {
      connectionVisualProgress = connectionTransitionTo;
      connectionTransitionActive = false;
    }
  }

  applyShellTransparency(connectionVisualProgress);
  applyModelConnectionPose(connectionVisualProgress);
}

function toMaterialList(mesh) {
  if (!mesh?.material) return [];
  return Array.isArray(mesh.material) ? mesh.material : [mesh.material];
}

function averageMaterialLuminance(mesh) {
  const materials = toMaterialList(mesh);
  if (!materials.length) return 1;
  let total = 0;
  let count = 0;
  materials.forEach((material) => {
    const color = material?.color;
    if (!color) return;
    total += (color.r + color.g + color.b) / 3;
    count += 1;
  });
  if (!count) return 1;
  return total / count;
}

function averageMaterialHsl(mesh) {
  const materials = toMaterialList(mesh);
  if (!materials.length) return null;

  let h = 0;
  let s = 0;
  let l = 0;
  let count = 0;
  const hsl = { h: 0, s: 0, l: 0 };
  materials.forEach((material) => {
    const color = material?.color;
    if (!color) return;
    color.getHSL(hsl);
    h += hsl.h;
    s += hsl.s;
    l += hsl.l;
    count += 1;
  });
  if (!count) return null;
  return { h: h / count, s: s / count, l: l / count };
}

function getMeshSize(mesh) {
  if (!mesh?.isMesh) return null;
  const bounds = new THREE.Box3().setFromObject(mesh);
  if (bounds.isEmpty()) return null;
  return bounds.getSize(new THREE.Vector3());
}

function computeMeshFingerprint(mesh, modelSize) {
  if (!mesh?.isMesh) return null;
  const bounds = new THREE.Box3().setFromObject(mesh);
  if (bounds.isEmpty()) return null;

  const size = bounds.getSize(new THREE.Vector3());
  const center = bounds.getCenter(new THREE.Vector3());
  const dims = [size.x, size.y, size.z].sort((a, b) => a - b);
  const low = Math.max(dims[0], 1e-9);
  const mid = Math.max(dims[1], 1e-9);
  const high = Math.max(dims[2], 1e-9);
  const modelVolume = Math.max(modelSize.x * modelSize.y * modelSize.z, 1e-9);
  const volume = Math.max(size.x * size.y * size.z, 0);
  const volumeRatio = volume / modelVolume;
  const materials = toMaterialList(mesh);
  const materialCount = Math.max(materials.length, 1);
  const opacitySum = materials.reduce((acc, material) => {
    if (!material || typeof material.opacity !== "number") return acc + 1;
    return acc + material.opacity;
  }, 0);

  const meshName = `${mesh.name || ""}`.trim();
  const parentName = `${mesh.parent?.name || ""}`.trim();
  const nodeName = `${meshName} ${parentName}`.trim();

  return {
    mesh,
    uuid: mesh.uuid,
    meshName,
    parentName,
    nodeName,
    center,
    size,
    volume,
    volumeRatio,
    roundness: mid / low,
    elongation: high / mid,
    dims: { low, mid, high },
    hsl: averageMaterialHsl(mesh),
    luminance: averageMaterialLuminance(mesh),
    opacity: opacitySum / materialCount,
    transparent: materials.some((material) => Boolean(material?.transparent)),
  };
}

function hasNamePattern(info, pattern) {
  return pattern.test(info.meshName) || pattern.test(info.parentName) || pattern.test(info.nodeName);
}

function parseCellIdFromInfo(info) {
  const sources = [info.meshName, info.parentName, info.nodeName].filter(Boolean);
  for (const source of sources) {
    const strictFromName = parseStrictCellIdFromName(source);
    if (strictFromName >= 1 && strictFromName <= CELL_COUNT) {
      return strictFromName;
    }
    const normalized = `${source}`;
    let match = normalized.match(/\bbattery\s*cell[\s_-]*(\d{1,2})\b/i);
    if (!match) {
      match = normalized.match(/\bcell[\s_-]*(\d{1,2})\b/i);
    }
    if (!match) continue;
    const parsed = Number.parseInt(match[1], 10);
    if (Number.isInteger(parsed) && parsed >= 1 && parsed <= CELL_COUNT) {
      return parsed;
    }
  }
  return null;
}

function parseFanBladeIdFromInfo(info) {
  const sources = [info.meshName, info.parentName, info.nodeName].filter(Boolean);
  for (const source of sources) {
    const strictFromName = parseStrictFanIdFromName(source);
    if (strictFromName === 1 || strictFromName === 2) {
      return strictFromName;
    }
    const normalized = `${source}`;
    let match = normalized.match(/\bfan[\s_-]*blade[\s_-]*(\d{1,2})\b/i);
    if (!match) {
      match = normalized.match(/\bblade[\s_-]*(\d{1,2})\b/i);
    }
    if (!match) continue;
    const parsed = Number.parseInt(match[1], 10);
    if (Number.isInteger(parsed) && parsed >= 1 && parsed <= 99) {
      return parsed;
    }
  }
  return null;
}

function normalizeNodeName(value) {
  return `${value || ""}`
    .toLowerCase()
    .replace(/_/g, " ")
    .replace(/[^a-z0-9]+/g, " ")
    .replace(/\s+/g, " ")
    .trim();
}

function parseStrictFanIdFromName(name) {
  const normalized = normalizeNodeName(name);
  const hasFan = /\bfan\b/i.test(normalized);
  const hasBlade = /\bblade\b/i.test(normalized);
  if (!hasFan || !hasBlade) return null;
  const match = normalized.match(/\bfan\b.*\bblade\b.*\b(1|2)\b/i);
  if (!match) return null;
  const parsed = Number.parseInt(match[1], 10);
  if (!Number.isInteger(parsed)) return null;
  return parsed;
}

function parseStrictCellIdFromName(name) {
  const normalized = normalizeNodeName(name);
  const hasCell = /\bcell\b/i.test(normalized);
  if (!hasCell) return null;
  let match = normalized.match(/\bbattery\b.*\bcell\b.*\b(\d{1,2})\b/i);
  if (!match) {
    match = normalized.match(/\bcell\b.*\b(\d{1,2})\b/i);
  }
  if (!match) return null;
  const parsed = Number.parseInt(match[1], 10);
  if (!Number.isInteger(parsed) || parsed < 1 || parsed > CELL_COUNT) return null;
  return parsed;
}

function nodeBoundsVolume(node) {
  if (!node) return 0;
  const bounds = new THREE.Box3().setFromObject(node);
  if (bounds.isEmpty()) return 0;
  const size = bounds.getSize(new THREE.Vector3());
  return Math.max(size.x * size.y * size.z, 0);
}

function nodeBoundsInfo(node) {
  if (!node) {
    return { volume: 0, center: new THREE.Vector3(), size: new THREE.Vector3(), empty: true };
  }
  const bounds = new THREE.Box3().setFromObject(node);
  if (bounds.isEmpty()) {
    return { volume: 0, center: new THREE.Vector3(), size: new THREE.Vector3(), empty: true };
  }
  const size = bounds.getSize(new THREE.Vector3());
  const center = bounds.getCenter(new THREE.Vector3());
  const volume = Math.max(size.x * size.y * size.z, 0);
  return { volume, center, size, empty: false };
}

function selectBestCellMesh(node) {
  if (!node) return null;
  if (node.isMesh) return node;

  let best = null;
  let bestScore = -Infinity;
  node.traverse((child) => {
    if (!child?.isMesh) return;
    const size = getMeshSize(child);
    if (!size) return;
    const dims = [size.x, size.y, size.z].sort((a, b) => a - b);
    const low = Math.max(dims[0], 1e-9);
    const mid = Math.max(dims[1], 1e-9);
    const high = Math.max(dims[2], 1e-9);
    const roundness = mid / low;
    const elongation = high / mid;
    const volume = size.x * size.y * size.z;
    if (volume <= 1e-12) return;

    const normalized = normalizeNodeName(`${child.name || ""} ${child.parent?.name || ""}`);
    const hasCell = /\bcell\b/i.test(normalized);
    const hasBattery = /\bbattery\b/i.test(normalized);
    const hasPcbLike = /\bpcb\b|\bboard\b/i.test(normalized);
    const hasFanLike = /\bfan\b|\bblade\b/i.test(normalized);
    const hasShellLike = /\bshell\b|\bcase\b|\bhousing\b|\blid\b/i.test(normalized);
    const hasHardwareLike = /\bscrew\b|\bbolt\b|\bnut\b|\bwasher\b|\bstandoff\b|\bconnector\b|\bpin\b/i.test(normalized);

    const materials = toMaterialList(child);
    const opacity = materials.length
      ? materials.reduce((acc, material) => acc + (typeof material?.opacity === "number" ? material.opacity : 1), 0) / materials.length
      : 1;

    let score = 0;
    if (hasCell) score += 300;
    if (hasBattery) score += 100;
    if (roundness >= 0.78 && roundness <= 1.35) score += 120;
    if (elongation >= 1.1 && elongation <= 6.0) score += 80;
    if (opacity >= 0.9) score += 40;
    score += Math.log10(Math.max(volume, 1e-12)) * 20;
    if (hasPcbLike) score -= 350;
    if (hasFanLike) score -= 250;
    if (hasShellLike) score -= 250;
    if (hasHardwareLike) score -= 200;

    if (score > bestScore) {
      bestScore = score;
      best = child;
    }
  });

  return best;
}

function selectStrictNamedCellObjects(rootObject, modelSize, boardPlane) {
  const groups = new Map(Array.from({ length: CELL_COUNT }, (_, idx) => [idx + 1, []]));
  const allowedRadius = Math.max(modelSize.length() * 1.8, 1.0);

  rootObject.traverse((child) => {
    if (!child || child === rootObject) return;
    const cellId = parseStrictCellIdFromName(child.name);
    if (!cellId) return;

    const boundsInfo = nodeBoundsInfo(child);
    if (boundsInfo.empty || boundsInfo.volume <= 1e-10) return;
    if (boundsInfo.center.length() > allowedRadius) return;

    const meshCount = meshDescendantCount(child);
    if (meshCount <= 0) return;

    let score = (boundsInfo.volume * 10) + (meshCount * 2);
    if (Number.isFinite(boardPlane?.y)) {
      const belowBoard = boundsInfo.center.y <= (boardPlane.y - (boardPlane?.margin || 0));
      score += belowBoard ? 80 : -500;
    }

    groups.get(cellId).push({
      node: child,
      score,
      volume: boundsInfo.volume,
      center: boundsInfo.center,
    });
  });

  const selected = [];
  for (let cellId = 1; cellId <= CELL_COUNT; cellId += 1) {
    const candidates = groups.get(cellId) || [];
    const best = candidates
      .sort((a, b) => (b.score - a.score) || (b.volume - a.volume))[0];
    if (!best?.node) continue;
    selected.push({ cellId, node: best.node });
  }

  return selected;
}

function buildEnclosureBounds(shellEntries) {
  if (!Array.isArray(shellEntries) || shellEntries.length === 0) return null;
  const combined = new THREE.Box3();
  let hasAny = false;
  shellEntries.forEach((entry) => {
    const mesh = entry?.mesh || entry;
    if (!mesh) return;
    const bounds = new THREE.Box3().setFromObject(mesh);
    if (bounds.isEmpty()) return;
    if (!hasAny) {
      combined.copy(bounds);
      hasAny = true;
    } else {
      combined.union(bounds);
    }
  });
  if (!hasAny) return null;

  const size = combined.getSize(new THREE.Vector3());
  const expand = Math.max(size.length() * 0.04, 0.03);
  return combined.clone().expandByScalar(expand);
}

function isInsideEnclosure(center, enclosureBounds) {
  if (!center || !enclosureBounds) return true;
  return enclosureBounds.containsPoint(center);
}

function meshDescendantCount(node) {
  if (!node) return 0;
  let count = 0;
  node.traverse((child) => {
    if (child?.isMesh) count += 1;
  });
  return count;
}

function selectStrictNamedFanBladeObjects(rootObject, enclosureBounds = null) {
  const groups = new Map([[1, []], [2, []]]);
  rootObject.traverse((child) => {
    if (!child || child === rootObject) return;
    const fanId = parseStrictFanIdFromName(child.name);
    if (fanId !== 1 && fanId !== 2) return;
    const boundsInfo = nodeBoundsInfo(child);
    const volume = boundsInfo.volume;
    const meshCount = meshDescendantCount(child);
    if (meshCount <= 0 || volume <= 1e-10 || boundsInfo.empty) return;
    if (!isInsideEnclosure(boundsInfo.center, enclosureBounds)) return;
    const score = (volume * 10) + (meshCount * 2);
    groups.get(fanId).push({
      node: child,
      score,
      volume,
      meshCount,
      centerX: boundsInfo.center.x,
    });
  });

  const group1 = (groups.get(1) || [])
    .sort((a, b) => (b.score - a.score) || (b.volume - a.volume))
    .slice(0, 8);
  const group2 = (groups.get(2) || [])
    .sort((a, b) => (b.score - a.score) || (b.volume - a.volume))
    .slice(0, 8);

  if (!group1.length || !group2.length) {
    return [];
  }

  let bestPair = null;
  let bestPairScore = -Infinity;
  group1.forEach((a) => {
    group2.forEach((b) => {
      if (!a?.node || !b?.node) return;
      if (a.node.uuid === b.node.uuid) return;
      const separation = Math.abs(a.centerX - b.centerX);
      const pairScore = a.score + b.score + (separation * 500);
      if (pairScore > bestPairScore) {
        bestPairScore = pairScore;
        bestPair = [a, b];
      }
    });
  });

  if (!bestPair) return [];
  return [
    { fanId: 1, node: bestPair[0].node },
    { fanId: 2, node: bestPair[1].node },
  ];
}

function selectLooseFanBladeObjects(rootObject, enclosureBounds = null) {
  const candidates = [];
  rootObject.traverse((child) => {
    if (!child || child === rootObject) return;
    const normalized = normalizeNodeName(child.name);
    if (!normalized.includes("fan") || !normalized.includes("blade")) return;
    const boundsInfo = nodeBoundsInfo(child);
    if (boundsInfo.empty || boundsInfo.volume <= 1e-10) return;
    if (!isInsideEnclosure(boundsInfo.center, enclosureBounds)) return;
    const meshCount = meshDescendantCount(child);
    if (meshCount <= 0) return;
    const fanId = parseStrictFanIdFromName(child.name);
    candidates.push({
      node: child,
      fanId: fanId === 1 || fanId === 2 ? fanId : null,
      volume: boundsInfo.volume,
      centerX: boundsInfo.center.x,
      meshCount,
    });
  });
  candidates.sort((a, b) => (b.volume - a.volume) || (b.meshCount - a.meshCount));
  return candidates;
}

function isLikelyCellGeometry(info) {
  return (
    info.roundness >= 0.82 &&
    info.roundness <= 1.22 &&
    info.elongation >= 1.25 &&
    info.elongation <= 4.2 &&
    info.volumeRatio >= 0.001 &&
    info.volumeRatio <= 0.05
  );
}

function isLikelyPcbInfo(info) {
  const hsl = info.hsl;
  const hasPcbName = hasNamePattern(info, PCB_NAME_PATTERN);
  const greenProfile = Boolean(hsl && hsl.h >= 0.20 && hsl.h <= 0.45 && hsl.s >= 0.25);
  const flatness = info.dims.mid / Math.max(info.dims.low, 1e-9);
  const spanRatio = info.dims.high / Math.max(info.dims.mid, 1e-9);
  const flatBoardShape = flatness >= 4.5 && spanRatio >= 1.2 && spanRatio <= 6.0;
  const mediumAreaVolume = info.volumeRatio >= 0.0002 && info.volumeRatio <= 0.03;
  return hasPcbName || (greenProfile && flatBoardShape && mediumAreaVolume);
}

function computeBoardPlane(meshInfoList, modelSize) {
  if (!Array.isArray(meshInfoList) || meshInfoList.length === 0) {
    return { y: null, margin: null, pcbInfos: [] };
  }

  const pcbInfos = meshInfoList.filter((info) => isLikelyPcbInfo(info));
  if (pcbInfos.length === 0) {
    return { y: null, margin: null, pcbInfos: [] };
  }

  const boardTopY = pcbInfos.reduce((acc, info) => {
    const topY = info.center.y + info.size.y * 0.5;
    return Math.max(acc, topY);
  }, -Infinity);
  const margin = Math.max(modelSize.y * BOARD_PLANE_MARGIN_RATIO, BOARD_PLANE_MARGIN_MIN);
  return {
    y: Number.isFinite(boardTopY) ? boardTopY : null,
    margin,
    pcbInfos,
  };
}

function scoreCellInfo(info, boardPlane) {
  let score = 0;
  if (hasNamePattern(info, CELL_NAME_PATTERN)) score += 180;
  if (isLikelyCellGeometry(info)) score += 90;

  const roundnessTarget = 1.0;
  const roundnessPenalty = Math.abs(info.roundness - roundnessTarget) * 120;
  score += Math.max(0, 60 - roundnessPenalty);

  const elongationTarget = 2.2;
  const elongationPenalty = Math.abs(info.elongation - elongationTarget) * 20;
  score += Math.max(0, 50 - elongationPenalty);

  if (boardPlane.y !== null) {
    if (info.center.y <= (boardPlane.y - boardPlane.margin)) score += 70;
    else score -= 180;
  }

  if (info.opacity >= 0.95 && !info.transparent) score += 35;
  else score -= 80;

  if (isLikelyPcbInfo(info)) score -= 200;
  if (hasNamePattern(info, FAN_NAME_PATTERN)) score -= 120;
  if (hasNamePattern(info, SHELL_NAME_PATTERN)) score -= 120;
  if (hasNamePattern(info, HARDWARE_NAME_PATTERN)) score -= 100;

  return score;
}

function buildCellCandidatePool(meshInfoList, boardPlane) {
  return meshInfoList
    .map((info) => ({ info, score: scoreCellInfo(info, boardPlane) }))
    .filter((entry) => {
      const { info } = entry;
      if (!isLikelyCellGeometry(info)) return false;
      if (isLikelyPcbInfo(info)) return false;
      if (hasNamePattern(info, FAN_NAME_PATTERN)) return false;
      if (hasNamePattern(info, SHELL_NAME_PATTERN)) return false;
      if (hasNamePattern(info, HARDWARE_NAME_PATTERN)) return false;
      if (info.opacity < 0.92) return false;
      if (boardPlane.y !== null && info.center.y > (boardPlane.y - boardPlane.margin)) return false;
      return true;
    });
}

function selectCellInfos(meshInfoList, boardPlane) {
  // Deterministic path: explicit CAD names like "Battery Cell 1..10".
  const explicitNamed = meshInfoList
    .filter((info) => {
      if (!info?.mesh?.isMesh) return false;
      if (!hasNamePattern(info, EXPLICIT_CELL_NAME_PATTERN)) return false;
      if (isLikelyPcbInfo(info)) return false;
      if (hasNamePattern(info, FAN_NAME_PATTERN)) return false;
      if (hasNamePattern(info, SHELL_NAME_PATTERN)) return false;
      if (hasNamePattern(info, HARDWARE_NAME_PATTERN)) return false;
      return true;
    })
    .map((info) => ({
      info,
      parsedId: parseCellIdFromInfo(info),
      score: scoreCellInfo(info, boardPlane),
    }));

  if (explicitNamed.length >= CELL_COUNT) {
    const byParsedId = explicitNamed.filter((entry) => Number.isInteger(entry.parsedId));
    if (byParsedId.length >= CELL_COUNT) {
      const dedupById = new Map();
      byParsedId
        .sort((a, b) => (b.score - a.score) || (b.info.volume - a.info.volume))
        .forEach((entry) => {
          if (!dedupById.has(entry.parsedId)) {
            dedupById.set(entry.parsedId, entry.info);
          }
        });
      const selectedInfos = Array.from({ length: CELL_COUNT }, (_, idx) => dedupById.get(idx + 1))
        .filter(Boolean);
      if (selectedInfos.length === CELL_COUNT) {
        return {
          selectedInfos,
          candidatePool: explicitNamed.map((entry) => ({ info: entry.info, score: entry.score })),
        };
      }
    }

    const selectedInfos = explicitNamed
      .sort((a, b) => (b.score - a.score) || (b.info.volume - a.info.volume))
      .slice(0, CELL_COUNT)
      .map((entry) => entry.info);

    selectedInfos.sort((a, b) => {
      const parsedA = parseCellIdFromInfo(a);
      const parsedB = parseCellIdFromInfo(b);
      if (parsedA !== null && parsedB !== null && parsedA !== parsedB) {
        return parsedA - parsedB;
      }
      const dz = b.center.z - a.center.z;
      if (Math.abs(dz) > 1e-3) return dz;
      const dx = a.center.x - b.center.x;
      if (Math.abs(dx) > 1e-3) return dx;
      return a.center.y - b.center.y;
    });

    return {
      selectedInfos,
      candidatePool: explicitNamed.map((entry) => ({ info: entry.info, score: entry.score })),
    };
  }

  const candidatePool = buildCellCandidatePool(meshInfoList, boardPlane);
  if (candidatePool.length === 0) {
    return { selectedInfos: [], candidatePool };
  }

  const volumeBins = new Map();
  candidatePool.forEach((candidate) => {
    const safeVolume = Math.max(candidate.info.volume, 1e-9);
    const logVolume = Math.log(safeVolume);
    const bin = Math.round(logVolume / 0.1);
    const entry = volumeBins.get(bin) || { count: 0, sumLog: 0 };
    entry.count += 1;
    entry.sumLog += logVolume;
    volumeBins.set(bin, entry);
  });

  const dominantBinEntry = [...volumeBins.entries()].sort((a, b) => b[1].count - a[1].count)[0];
  let refinedPool = candidatePool;
  if (dominantBinEntry) {
    const dominantCenterLog = dominantBinEntry[1].sumLog / Math.max(dominantBinEntry[1].count, 1);
    refinedPool = candidatePool.filter((candidate) => {
      const logVolume = Math.log(Math.max(candidate.info.volume, 1e-9));
      return Math.abs(logVolume - dominantCenterLog) <= 0.24;
    });
    if (refinedPool.length < CELL_COUNT) {
      refinedPool = candidatePool;
    }
  }

  const selectedInfos = refinedPool
    .sort((a, b) => (b.score - a.score) || (b.info.volume - a.info.volume))
    .slice(0, CELL_COUNT)
    .map((candidate) => candidate.info);

  // Deterministic ordering for stable ID assignment.
  selectedInfos.sort((a, b) => {
    const dz = b.center.z - a.center.z;
    if (Math.abs(dz) > 1e-3) return dz;
    const dx = a.center.x - b.center.x;
    if (Math.abs(dx) > 1e-3) return dx;
    return a.center.y - b.center.y;
  });

  return { selectedInfos, candidatePool };
}

function selectShellInfos(meshInfoList, modelSize, cellUuidSet) {
  const candidates = meshInfoList
    .filter((info) => {
      if (!info?.mesh?.isMesh) return false;
      if (cellUuidSet.has(info.uuid)) return false;
      if (isLikelyPcbInfo(info)) return false;
      if (isLikelyCellGeometry(info)) return false;
      if (hasNamePattern(info, FAN_NAME_PATTERN)) return false;
      if (hasNamePattern(info, HARDWARE_NAME_PATTERN)) return false;
      const nameHasShell = hasNamePattern(info, SHELL_NAME_PATTERN);
      const geometryLooksShell = isLikelyShellMesh(info.mesh, modelSize);
      return nameHasShell || geometryLooksShell;
    })
    .map((info) => {
      const nameHasShell = hasNamePattern(info, SHELL_NAME_PATTERN);
      const geometryLooksShell = isLikelyShellMesh(info.mesh, modelSize);
      let score = 0;
      if (nameHasShell) score += 120;
      if (geometryLooksShell) score += 70;
      if (info.luminance <= 0.3) score += 25;
      if (info.volumeRatio >= 0.01) score += 25;
      return { info, score };
    })
    .sort((a, b) => (b.score - a.score) || (b.info.volume - a.info.volume))
    .slice(0, SHELL_TARGET_MAX)
    .map((entry) => entry.info);

  if (candidates.length > 0) {
    return candidates;
  }

  // Fallback: dark outer envelope meshes only.
  return meshInfoList
    .filter((info) => {
      if (cellUuidSet.has(info.uuid)) return false;
      if (isLikelyPcbInfo(info)) return false;
      if (isLikelyCellGeometry(info)) return false;
      return isLikelyShellMesh(info.mesh, modelSize);
    })
    .sort((a, b) => b.volume - a.volume)
    .slice(0, SHELL_TARGET_MAX);
}

function selectShellMeshesByName(rootObject, cellUuidSet) {
  const selected = [];
  const seen = new Set();
  rootObject.traverse((child) => {
    if (!child?.isMesh) return;
    if (cellUuidSet.has(child.uuid)) return;
    if (seen.has(child.uuid)) return;
    const label = `${child.name || ""} ${child.parent?.name || ""}`;
    if (!SHELL_NAME_PATTERN.test(label) && !FAN_SHELL_NAME_PATTERN.test(label)) return;
    seen.add(child.uuid);
    selected.push(child);
  });
  return selected;
}

function isLikelyShellMesh(mesh, modelSize) {
  const size = getMeshSize(mesh);
  if (!size || !modelSize) return false;
  const luminance = averageMaterialLuminance(mesh);
  if (luminance > 0.24) return false;

  const rx = size.x / Math.max(modelSize.x, 1e-6);
  const ry = size.y / Math.max(modelSize.y, 1e-6);
  const rz = size.z / Math.max(modelSize.z, 1e-6);
  const ratios = [rx, ry, rz].sort((a, b) => b - a);
  // Need coverage across the pack envelope, but avoid tiny dark internals.
  return ratios[0] >= 0.7 && ratios[1] >= 0.45;
}

function isLikelyFanBladeMesh(mesh, modelSize) {
  const size = getMeshSize(mesh);
  if (!size || !modelSize) return false;
  const hsl = averageMaterialHsl(mesh);
  if (!hsl) return false;

  // Typical blade colors in this model are yellow/green.
  const hueOk = hsl.h >= 0.08 && hsl.h <= 0.22;
  const satOk = hsl.s >= 0.2;
  if (!hueOk || !satOk) return false;

  const dims = [size.x, size.y, size.z].sort((a, b) => a - b);
  const low = dims[0];
  const high = dims[2];
  if (low <= 0 || high <= 0) return false;
  const flatness = high / low;

  const modelVolume = Math.max(modelSize.x * modelSize.y * modelSize.z, 1e-6);
  const meshVolume = size.x * size.y * size.z;
  const volumeRatio = meshVolume / modelVolume;

  return flatness >= 1.8 && volumeRatio >= 0.00005 && volumeRatio <= 0.01;
}

function registerShellMaterialState(mesh) {
  toMaterialList(mesh).forEach((material) => {
    if (!material || typeof material.opacity !== "number") return;
    if (material.userData.bmsShellBaseOpacity === undefined) {
      material.userData.bmsShellBaseOpacity = material.opacity;
      material.userData.bmsShellBaseTransparent = Boolean(material.transparent);
      material.userData.bmsShellBaseDepthWrite = material.depthWrite !== false;
    }
  });
}

function detectFanSpinAxis(mesh) {
  const geometry = mesh?.geometry;
  if (!geometry) return "z";
  if (!geometry.boundingBox) {
    geometry.computeBoundingBox();
  }
  const bounds = geometry.boundingBox;
  if (!bounds) return "z";
  const size = bounds.getSize(new THREE.Vector3());
  if (size.x <= size.y && size.x <= size.z) return "x";
  if (size.y <= size.x && size.y <= size.z) return "y";
  return "z";
}

function axisVectorForLabel(axisLabel) {
  if (axisLabel === "x") return new THREE.Vector3(1, 0, 0);
  if (axisLabel === "y") return new THREE.Vector3(0, 1, 0);
  return new THREE.Vector3(0, 0, 1);
}

function centerMeshGeometryForSpin(mesh) {
  if (!mesh?.isMesh || !mesh.geometry) return;
  if (mesh.userData?.bmsSpinCentered) return;

  const sourceGeometry = mesh.geometry;
  const geometry = sourceGeometry.clone();
  if (!geometry.boundingBox) {
    geometry.computeBoundingBox();
  }
  const bounds = geometry.boundingBox;
  if (!bounds || bounds.isEmpty()) {
    mesh.geometry = geometry;
    mesh.userData.bmsSpinCentered = true;
    return;
  }

  const center = bounds.getCenter(new THREE.Vector3());
  // If geometry already centered, nothing to do.
  if (center.lengthSq() <= 1e-12) {
    mesh.geometry = geometry;
    mesh.userData.bmsSpinCentered = true;
    return;
  }

  geometry.translate(-center.x, -center.y, -center.z);
  geometry.computeBoundingBox();
  geometry.computeBoundingSphere();
  mesh.geometry = geometry;

  // Compensate mesh position so world-space placement stays unchanged.
  const offsetParentSpace = center
    .clone()
    .multiply(mesh.scale)
    .applyQuaternion(mesh.quaternion);
  mesh.position.add(offsetParentSpace);
  mesh.updateMatrixWorld(true);

  mesh.userData.bmsSpinCentered = true;
  mesh.userData.bmsSpinCenter = center;
}

function createFanSpinEntry(mesh, forcedAxisLabel = null, fanId = null) {
  if (!mesh?.isMesh) {
    return null;
  }

  centerMeshGeometryForSpin(mesh);

  const axis = forcedAxisLabel || detectFanSpinAxis(mesh);
  const axisVecMeshLocal = axisVectorForLabel(axis);
  const worldPos = mesh.getWorldPosition(new THREE.Vector3());

  return {
    mesh,
    spinNode: mesh,
    axis,
    axisVector: axisVecMeshLocal.clone().normalize(),
    rpm: 0,
    worldX: worldPos.x,
    fanId,
  };
}

function detectObjectSpinAxis(node) {
  if (!node) return "z";
  if (node.isMesh) return detectFanSpinAxis(node);

  let bestMesh = null;
  let bestScore = -Infinity;
  node.traverse((child) => {
    if (!child?.isMesh) return;
    const size = getMeshSize(child);
    if (!size) return;
    const volume = size.x * size.y * size.z;
    const normalized = normalizeNodeName(`${child.name || ""} ${child.parent?.name || ""}`);
    const hasBladeToken = /\bblade\b/i.test(normalized);
    const hasFanToken = /\bfan\b/i.test(normalized);
    const hasShellToken = /\bshell\b|\bframe\b|\bhousing\b/i.test(normalized);
    let score = volume;
    if (hasBladeToken) score += 1000;
    if (hasFanToken) score += 120;
    if (hasShellToken) score -= 800;
    if (score > bestScore) {
      bestScore = score;
      bestMesh = child;
    }
  });

  if (bestMesh) {
    return detectFanSpinAxis(bestMesh);
  }
  return "z";
}

function selectBestBladeMesh(node) {
  if (!node) return null;
  if (node.isMesh) return node;

  let best = null;
  let bestScore = -Infinity;
  node.traverse((child) => {
    if (!child?.isMesh) return;
    const size = getMeshSize(child);
    if (!size) return;
    const volume = size.x * size.y * size.z;
    if (volume <= 1e-12) return;

    const dims = [size.x, size.y, size.z].sort((a, b) => a - b);
    const low = Math.max(dims[0], 1e-9);
    const mid = Math.max(dims[1], 1e-9);
    const high = Math.max(dims[2], 1e-9);
    const flatness = high / low;
    const spread = mid / low;

    const normalized = normalizeNodeName(`${child.name || ""} ${child.parent?.name || ""}`);
    const hasBlade = /\bblade\b/i.test(normalized);
    const hasFan = /\bfan\b/i.test(normalized);
    const hasShellLike = /\bshell\b|\bframe\b|\bhousing\b/i.test(normalized);

    let score = 0;
    score += Math.log10(Math.max(volume, 1e-12)) * 40;
    if (hasBlade) score += 250;
    if (hasFan) score += 60;
    if (hasShellLike) score -= 200;
    if (flatness >= 1.8) score += 55;
    if (flatness >= 2.6) score += 40;
    if (spread >= 1.3) score += 25;

    if (score > bestScore) {
      bestScore = score;
      best = child;
    }
  });

  return best;
}

function createObjectSpinEntry(node, forcedAxisLabel = null, fanId = null) {
  if (!node) {
    return null;
  }

  // Prefer spinning the actual blade mesh inside the named fan object.
  const bladeMesh = selectBestBladeMesh(node);
  if (bladeMesh) {
    const bladeAxis = forcedAxisLabel || detectFanSpinAxis(bladeMesh);
    const meshEntry = createFanSpinEntry(bladeMesh, bladeAxis, fanId);
    if (meshEntry) {
      meshEntry.objectName = node.name || bladeMesh.name || "";
      return meshEntry;
    }
  }

  const axis = forcedAxisLabel || detectObjectSpinAxis(node);
  const axisVecObjectLocal = axisVectorForLabel(axis);
  const worldBounds = new THREE.Box3().setFromObject(node);
  if (worldBounds.isEmpty()) return null;
  const worldCenter = worldBounds.getCenter(new THREE.Vector3());

  const parent = node.parent;
  if (!parent) {
    const mesh = node?.isMesh ? node : node.getObjectByProperty("isMesh", true);
    return {
      mesh: mesh || null,
      objectName: node.name || "",
      spinNode: node,
      axis,
      axisVector: axisVecObjectLocal.clone().normalize(),
      rpm: 0,
      worldX: worldCenter.x,
      fanId,
    };
  }

  const nodeWorldQuat = node.getWorldQuaternion(new THREE.Quaternion());
  const worldAxis = axisVecObjectLocal.clone().applyQuaternion(nodeWorldQuat).normalize();

  parent.updateMatrixWorld(true);
  const localCenter = parent.worldToLocal(worldCenter.clone());

  const pivot = new THREE.Group();
  pivot.name = `BMSFanPivot_${fanId || node.name || node.uuid}`;
  pivot.position.copy(localCenter);
  parent.add(pivot);
  pivot.updateMatrixWorld(true);
  pivot.attach(node);
  pivot.updateMatrixWorld(true);

  const pivotWorldQuat = pivot.getWorldQuaternion(new THREE.Quaternion());
  const pivotAxis = worldAxis
    .clone()
    .applyQuaternion(pivotWorldQuat.clone().invert())
    .normalize();

  const mesh = node?.isMesh ? node : node.getObjectByProperty("isMesh", true);
  return {
    mesh: mesh || null,
    objectName: node.name || "",
    spinNode: pivot,
    axis,
    axisVector: pivotAxis,
    rpm: 0,
    worldX: worldCenter.x,
    fanId,
  };
}

function applyShellTransparency(connectedBlend) {
  const clampedBlend = THREE.MathUtils.clamp(connectedBlend, 0, 1);

  // Apply to BMS shells
  shellMeshes.forEach((entry) => {
    toMaterialList(entry.mesh).forEach((material) => {
      if (!material || typeof material.opacity !== "number") return;

      const baseOpacity = material.userData.bmsShellBaseOpacity ?? 1;
      const baseTransparent = Boolean(material.userData.bmsShellBaseTransparent);
      const baseDepthWrite = material.userData.bmsShellBaseDepthWrite !== false;
      const transparentTarget = Math.min(baseOpacity, SHELL_CONNECTED_OPACITY);
      const targetOpacity = THREE.MathUtils.lerp(baseOpacity, transparentTarget, clampedBlend);

      const shouldBeTransparent = baseTransparent || targetOpacity < 0.995;
      const shouldDepthWrite = shouldBeTransparent ? false : baseDepthWrite;

      if (material.transparent !== shouldBeTransparent) {
        material.transparent = shouldBeTransparent;
        material.needsUpdate = true;
      }
      material.depthWrite = shouldDepthWrite;
      material.opacity = targetOpacity;
    });
  });

  // E-Load shell/lid transparency is now handled by the independent
  // eload reveal transition system (applyEloadRevealTransparency).
}

function collectFanCandidate(mesh, nodeName, modelSize) {
  const size = getMeshSize(mesh);
  if (!size || !modelSize) return null;

  const modelVolume = Math.max(modelSize.x * modelSize.y * modelSize.z, 1e-6);
  const volume = size.x * size.y * size.z;
  const volumeRatio = volume / modelVolume;
  const hsl = averageMaterialHsl(mesh);

  const nameHasFan = FAN_NAME_PATTERN.test(nodeName);
  const nameHasBlade = FAN_BLADE_NAME_PATTERN.test(nodeName);
  const nameHasFanShell = FAN_SHELL_NAME_PATTERN.test(nodeName);
  const geometryLikelyBlade = isLikelyFanBladeMesh(mesh, modelSize);
  const hueLikelyBlade = Boolean(hsl && hsl.h >= 0.08 && hsl.h <= 0.22 && hsl.s >= 0.2);

  if (nameHasFanShell && !nameHasBlade && !geometryLikelyBlade) return null;
  if (!nameHasFan && !geometryLikelyBlade) return null;

  let score = 0;
  if (nameHasBlade) score += 130;
  else if (nameHasFan) score += 40;
  if (geometryLikelyBlade) score += 80;
  if (hueLikelyBlade) score += 40;

  if (SHELL_NAME_PATTERN.test(nodeName)) score -= 70;
  if (nameHasFanShell && !nameHasBlade) score -= 140;
  if (CELL_NAME_PATTERN.test(nodeName)) score -= 40;
  if (volumeRatio >= 0.0001 && volumeRatio <= 0.02) score += 25;

  return {
    mesh,
    score,
    volume,
    parentKey: mesh.parent?.uuid || "",
  };
}

function finalizeFanMeshes(fanCandidates, object, modelSize) {
  const selected = [];
  const usedMeshes = new Set();
  const usedParents = new Set();

  fanCandidates
    .sort((a, b) => (b.score - a.score) || (b.volume - a.volume))
    .forEach((candidate) => {
      if (selected.length >= FAN_TARGET_COUNT) return;
      if (!candidate?.mesh) return;
      if (usedMeshes.has(candidate.mesh.uuid)) return;
      if (candidate.parentKey && usedParents.has(candidate.parentKey)) return;

      usedMeshes.add(candidate.mesh.uuid);
      if (candidate.parentKey) {
        usedParents.add(candidate.parentKey);
      }
      const entry = createFanSpinEntry(candidate.mesh);
      if (entry) {
        selected.push(entry);
      }
    });

  if (selected.length > 0) {
    return selected;
  }

  // Fallback for CAD exports with unexpected names.
  const fallback = [];
  object.traverse((child) => {
    if (!child.isMesh || fallback.length >= FAN_TARGET_COUNT) return;
    if (!isLikelyFanBladeMesh(child, modelSize)) return;
    const entry = createFanSpinEntry(child);
    if (entry) {
      fallback.push(entry);
    }
  });
  return fallback;
}

function selectStrictNamedFanBlades(meshInfoList) {
  const groups = new Map([[1, []], [2, []]]);
  meshInfoList.forEach((info) => {
    if (!info?.mesh?.isMesh) return;
    if (hasNamePattern(info, FAN_SHELL_NAME_PATTERN)) return;
    if (hasNamePattern(info, SHELL_NAME_PATTERN)) return;
    if (hasNamePattern(info, PCB_NAME_PATTERN)) return;
    if (hasNamePattern(info, CELL_NAME_PATTERN)) return;

    const parsedId = parseFanBladeIdFromInfo(info);
    if (parsedId !== 1 && parsedId !== 2) return;

    const meshNorm = normalizeNodeName(info.meshName);
    const parentNorm = normalizeNodeName(info.parentName);
    const nodeNorm = normalizeNodeName(info.nodeName);
    const exactTagA = `bms-fan blade ${parsedId}`;
    const exactTagB = `bms - fan blade ${parsedId}`;
    const exactMatch =
      meshNorm.includes(exactTagA) ||
      meshNorm.includes(exactTagB) ||
      parentNorm.includes(exactTagA) ||
      parentNorm.includes(exactTagB) ||
      nodeNorm.includes(exactTagA) ||
      nodeNorm.includes(exactTagB);

    const centerDistance = info.center.length();
    let score = 0;
    if (exactMatch) score += 500;
    if (hasNamePattern(info, EXPLICIT_FAN_BLADE_NAME_PATTERN)) score += 200;
    if (hasNamePattern(info, FAN_BLADE_NAME_PATTERN)) score += 80;
    if (info.volumeRatio >= 0.00001 && info.volumeRatio <= 0.03) score += 30;
    score -= centerDistance * 5;

    groups.get(parsedId).push({ info, score });
  });

  const selected = [];
  [1, 2].forEach((fanId) => {
    const best = (groups.get(fanId) || [])
      .sort((a, b) => (b.score - a.score) || (b.info.volume - a.info.volume))[0];
    if (best?.info) {
      selected.push({ fanId, info: best.info });
    }
  });
  return selected;
}

// --- Load Model ---
const MODEL_PROCESS_SEGMENTS = {
  prepare: { start: 0.00, span: 0.10 },
  traverse: { start: 0.10, span: 0.30 },
  cells: { start: 0.40, span: 0.20 },
  shell: { start: 0.60, span: 0.10 },
  fans: { start: 0.70, span: 0.20 },
  finalize: { start: 0.90, span: 0.10 },
};

async function initializeLoadedModel(object) {
  loadedModel = object;
  setBootStage("modelProcess", "Processing 3D model...");
  setBootDetail("Preparing geometry...");
  setBootStageProgress("modelDownload", 1);
  setBootStageProgress("modelProcess", 0);

  // Prepare bounds and model pose.
  const box = new THREE.Box3().setFromObject(object);
  const center = box.getCenter(new THREE.Vector3());
  const modelSize = box.getSize(new THREE.Vector3());
  object.position.sub(center); // Center at 0,0,0
  configureModelPoseTargets(object);
  setModelProcessProgress(MODEL_PROCESS_SEGMENTS.prepare.start, MODEL_PROCESS_SEGMENTS.prepare.span, 1);
  await waitForNextFrame();

  // Reset in case model reloads.
  meshInfos.length = 0;
  selectedCellUuidSet.clear();
  cellMeshes.length = 0;
  shellMeshes.length = 0;
  fanMeshes.length = 0;
  boardMeshes.length = 0;
  connectorMeshes.length = 0;

  const fastNamedCellObjects = FAST_MODEL_INIT
    ? selectStrictNamedCellObjects(object, modelSize, { y: null, margin: 0, pcbInfos: [] })
    : [];
  const useFastNamedPath = FAST_MODEL_INIT && fastNamedCellObjects.length === CELL_COUNT;

  // Traverse and gather fingerprints/candidates.
  setBootDetail("Scanning model meshes...");
  const meshNodes = [];
  object.traverse((child) => {
    if (child?.isMesh) {
      meshNodes.push(child);
    }
  });
  const meshTotal = Math.max(meshNodes.length, 1);
  const fanCandidates = [];
  for (let idx = 0; idx < meshNodes.length; idx += 1) {
    const child = meshNodes[idx];
    child.castShadow = false;
    child.receiveShadow = false;

    // Fix invalid material indices (negative values).
    if (child.geometry && child.geometry.groups) {
      child.geometry.groups.forEach((group) => {
        if (group.materialIndex < 0) group.materialIndex = 0;
      });
    }

    if (!useFastNamedPath) {
      const info = computeMeshFingerprint(child, modelSize);
      if (info) {
        meshInfos.push(info);
        const fanCandidate = collectFanCandidate(child, info.nodeName, modelSize);
        if (fanCandidate) {
          fanCandidates.push(fanCandidate);
        }
      }
    }

    const traverseProgress = (idx + 1) / meshTotal;
    setModelProcessProgress(
      MODEL_PROCESS_SEGMENTS.traverse.start,
      MODEL_PROCESS_SEGMENTS.traverse.span,
      traverseProgress,
    );
    if ((idx + 1) % 120 === 0) {
      await waitForNextFrame();
    }
  }
  if (meshNodes.length === 0) {
    setModelProcessProgress(MODEL_PROCESS_SEGMENTS.traverse.start, MODEL_PROCESS_SEGMENTS.traverse.span, 1);
  }

  // Cell selection/material setup.
  setBootDetail("Selecting battery cells...");
  const boardPlane = useFastNamedPath
    ? { y: null, margin: null, pcbInfos: [] }
    : computeBoardPlane(meshInfos, modelSize);
  let cellCandidateCount = 0;
  const strictCellObjects = useFastNamedPath
    ? fastNamedCellObjects
    : selectStrictNamedCellObjects(object, modelSize, boardPlane);
  if (strictCellObjects.length === CELL_COUNT) {
    strictCellObjects
      .sort((a, b) => a.cellId - b.cellId)
      .forEach(({ cellId, node }) => {
        const mesh = selectBestCellMesh(node);
        if (!mesh) return;
        selectedCellUuidSet.add(mesh.uuid);

        // Clone so only cell meshes are colorized by telemetry.
        if (Array.isArray(mesh.material)) {
          mesh.material = mesh.material.map((material) => (
            material?.clone ? material.clone() : material
          ));
        } else if (mesh.material?.clone) {
          mesh.material = mesh.material.clone();
        }

        // Force cells to remain opaque even when shell is transparent.
        toMaterialList(mesh).forEach((material) => {
          if (!material || typeof material.opacity !== "number") return;
          material.transparent = false;
          material.opacity = 1;
          material.depthWrite = true;
          material.needsUpdate = true;
        });

        const probeMaterial = Array.isArray(mesh.material) ? mesh.material[0] : mesh.material;
        const baseColor = probeMaterial?.color ? probeMaterial.color.clone() : new THREE.Color(0.55, 0.55, 0.55);
        cellMeshes.push({
          id: cellId,
          mesh,
          baseColor,
          targetColor: baseColor.clone(),
        });
      });
    cellCandidateCount = strictCellObjects.length;
  } else {
    const cellSelection = selectCellInfos(meshInfos, boardPlane);
    cellSelection.selectedInfos.forEach((info, idx) => {
      const mesh = info.mesh;
      if (!mesh) return;
      selectedCellUuidSet.add(info.uuid);

      // Clone so only cell meshes are colorized by telemetry.
      if (Array.isArray(mesh.material)) {
        mesh.material = mesh.material.map((material) => (
          material?.clone ? material.clone() : material
        ));
      } else if (mesh.material?.clone) {
        mesh.material = mesh.material.clone();
      }

      // Force cells to remain opaque even when shell is transparent.
      toMaterialList(mesh).forEach((material) => {
        if (!material || typeof material.opacity !== "number") return;
        material.transparent = false;
        material.opacity = 1;
        material.depthWrite = true;
        material.needsUpdate = true;
      });

      const probeMaterial = Array.isArray(mesh.material) ? mesh.material[0] : mesh.material;
      const baseColor = probeMaterial?.color ? probeMaterial.color.clone() : new THREE.Color(0.55, 0.55, 0.55);
      cellMeshes.push({
        id: idx + 1,
        mesh,
        baseColor,
        targetColor: baseColor.clone(),
      });
    });
    cellCandidateCount = cellSelection.candidatePool.length;
  }
  setModelProcessProgress(MODEL_PROCESS_SEGMENTS.cells.start, MODEL_PROCESS_SEGMENTS.cells.span, 1);
  await waitForNextFrame();

  if (cellMeshes.length !== CELL_COUNT) {
    console.warn(
      `[BMS] Expected ${CELL_COUNT} cells but selected ${cellMeshes.length}.`,
    );
  }
  cellMeshes.sort((a, b) => a.id - b.id);

  // Shell mesh selection/material setup.
  setBootDetail("Preparing enclosure meshes...");
  if (useFastNamedPath) {
    selectShellMeshesByName(object, selectedCellUuidSet).forEach((mesh) => {
      if (!mesh) return;
      shellMeshes.push({ mesh });
      registerShellMaterialState(mesh);
    });
  } else {
    selectShellInfos(meshInfos, modelSize, selectedCellUuidSet).forEach((info) => {
      if (!info?.mesh) return;
      shellMeshes.push({ mesh: info.mesh });
      registerShellMaterialState(info.mesh);
    });
  }
  setModelProcessProgress(MODEL_PROCESS_SEGMENTS.shell.start, MODEL_PROCESS_SEGMENTS.shell.span, 1);
  await waitForNextFrame();

  // Fan selection/finalization.
  setBootDetail("Preparing fan components...");
  const enclosureBounds = buildEnclosureBounds(shellMeshes);

  let strictFanObjects = selectStrictNamedFanBladeObjects(object, enclosureBounds);
  if (strictFanObjects.length < FAN_TARGET_COUNT) {
    strictFanObjects = selectStrictNamedFanBladeObjects(object, null);
  }
  const usedFanObjectUuids = new Set();
  strictFanObjects.forEach(({ fanId, node }) => {
    const entry = createObjectSpinEntry(node, null, fanId);
    if (!entry) return;
    if (usedFanObjectUuids.has(node.uuid)) return;
    usedFanObjectUuids.add(node.uuid);
    fanMeshes.push(entry);
  });
  setModelProcessProgress(MODEL_PROCESS_SEGMENTS.fans.start, MODEL_PROCESS_SEGMENTS.fans.span, 0.35);

  if (fanMeshes.length < FAN_TARGET_COUNT) {
    let looseCandidates = selectLooseFanBladeObjects(object, enclosureBounds);
    if (looseCandidates.length === 0) {
      looseCandidates = selectLooseFanBladeObjects(object, null);
    }
    const existingFanIds = new Set(
      fanMeshes.map((entry) => entry?.fanId).filter((id) => id === 1 || id === 2),
    );

    // Fill missing explicit IDs first.
    looseCandidates.forEach((candidate) => {
      if (fanMeshes.length >= FAN_TARGET_COUNT) return;
      if (!candidate?.node) return;
      if (usedFanObjectUuids.has(candidate.node.uuid)) return;
      if (candidate.fanId !== 1 && candidate.fanId !== 2) return;
      if (existingFanIds.has(candidate.fanId)) return;
      const entry = createObjectSpinEntry(candidate.node, null, candidate.fanId);
      if (!entry) return;
      usedFanObjectUuids.add(candidate.node.uuid);
      existingFanIds.add(candidate.fanId);
      fanMeshes.push(entry);
    });

    // Last resort: add farthest-side blade-like candidate.
    if (fanMeshes.length < FAN_TARGET_COUNT) {
      const referenceX = fanMeshes.length
        ? fanMeshes.reduce((acc, item) => acc + (Number.isFinite(item?.worldX) ? item.worldX : 0), 0) / fanMeshes.length
        : 0;
      const fallback = looseCandidates
        .filter((candidate) => candidate?.node && !usedFanObjectUuids.has(candidate.node.uuid))
        .sort((a, b) => Math.abs(b.centerX - referenceX) - Math.abs(a.centerX - referenceX))[0];
      if (fallback?.node) {
        const guessedFanId = fallback.centerX < referenceX ? 1 : 2;
        const entry = createObjectSpinEntry(fallback.node, null, guessedFanId);
        if (entry) {
          usedFanObjectUuids.add(fallback.node.uuid);
          fanMeshes.push(entry);
        }
      }
    }
  }
  setModelProcessProgress(MODEL_PROCESS_SEGMENTS.fans.start, MODEL_PROCESS_SEGMENTS.fans.span, 0.65);

  if (!useFastNamedPath && fanMeshes.length < FAN_TARGET_COUNT) {
    const explicitFanBladeInfos = selectStrictNamedFanBlades(meshInfos);
    const usedFanMeshUuids = new Set();
    const existingFanIds = new Set(fanMeshes.map((entry) => entry?.fanId).filter((id) => id === 1 || id === 2));
    fanMeshes.forEach((entry) => {
      if (entry?.mesh?.uuid) usedFanMeshUuids.add(entry.mesh.uuid);
    });
    explicitFanBladeInfos.forEach(({ fanId, info }) => {
      if (fanId === 1 || fanId === 2) {
        if (existingFanIds.has(fanId)) return;
      }
      const entry = createFanSpinEntry(info.mesh, null, fanId);
      if (!entry || !entry.mesh) return;
      if (usedFanMeshUuids.has(entry.mesh.uuid)) return;
      usedFanMeshUuids.add(entry.mesh.uuid);
      fanMeshes.push(entry);
      if (fanId === 1 || fanId === 2) {
        existingFanIds.add(fanId);
      }
    });
  }

  if (fanMeshes.length < FAN_TARGET_COUNT) {
    const existingMeshUuids = new Set(
      fanMeshes
        .map((entry) => entry?.mesh?.uuid)
        .filter((uuid) => typeof uuid === "string" && uuid.length > 0),
    );
    finalizeFanMeshes(fanCandidates, object, modelSize).forEach((entry) => {
      if (!entry?.mesh?.uuid) return;
      if (fanMeshes.length >= FAN_TARGET_COUNT) return;
      if (existingMeshUuids.has(entry.mesh.uuid)) return;
      existingMeshUuids.add(entry.mesh.uuid);
      fanMeshes.push(entry);
    });
  }
  setModelProcessProgress(MODEL_PROCESS_SEGMENTS.fans.start, MODEL_PROCESS_SEGMENTS.fans.span, 1);
  await waitForNextFrame();

  // Board / connector mesh identification
  setBootDetail("Identifying ports and connectors...");
  const classifiedUuids = new Set();
  cellMeshes.forEach(e => classifiedUuids.add(e.mesh?.uuid));
  shellMeshes.forEach(e => classifiedUuids.add(e.mesh?.uuid));
  fanMeshes.forEach(e => {
    if (e.mesh?.uuid) classifiedUuids.add(e.mesh.uuid);
    if (e.spinNode) e.spinNode.traverse(c => { if (c.isMesh) classifiedUuids.add(c.uuid); });
  });

  object.traverse((child) => {
    if (!child?.isMesh) return;
    if (classifiedUuids.has(child.uuid)) return;
    const name = (child.name || '').toLowerCase();
    const parentName = (child.parent?.name || '').toLowerCase();
    const combined = `${name} ${parentName}`;
    if (PCB_NAME_PATTERN.test(combined)) {
      boardMeshes.push(child);
      classifiedUuids.add(child.uuid);
    } else if (HARDWARE_NAME_PATTERN.test(combined)) {
      connectorMeshes.push(child);
      classifiedUuids.add(child.uuid);
    }
  });
  console.log(`[BMS] Board meshes: ${boardMeshes.length}, Connector meshes: ${connectorMeshes.length}`);

  // Final model state + camera fit.
  setBootDetail("Finalizing 3D scene...");
  lastModelSelectionDebug = {
    boardPlaneY: boardPlane.y,
    boardPlaneMargin: boardPlane.margin,
    fastInitPath: useFastNamedPath,
    meshInfoCount: meshInfos.length,
    cellCandidateCount,
    selectedCellCount: cellMeshes.length,
    shellCount: shellMeshes.length,
    fanCount: fanMeshes.length,
    selectedCells: cellMeshes.map((entry) => {
      const info = meshInfos.find((item) => item.uuid === entry.mesh?.uuid);
      return {
        id: entry.id,
        uuid: entry.mesh?.uuid || "",
        name: entry.mesh?.name || "",
        parentName: entry.mesh?.parent?.name || "",
        center: info
          ? {
            x: Number(info.center.x.toFixed(3)),
            y: Number(info.center.y.toFixed(3)),
            z: Number(info.center.z.toFixed(3)),
          }
          : null,
      };
    }),
    selectedFans: fanMeshes.map((entry) => ({
      name: entry.objectName || entry.mesh?.name || "",
      parentName: entry.mesh?.parent?.name || "",
      spinNode: entry.spinNode?.name || "",
      axis: entry.axis || "",
      fanId: entry.fanId ?? null,
      side: Number.isFinite(entry.worldX) ? (entry.worldX <= 0 ? "left-ish" : "right-ish") : "unknown",
      rpm: Number(entry.rpm || 0),
      meshName: entry.mesh?.name || "",
      x: Number((entry.worldX ?? entry.mesh?.getWorldPosition?.(new THREE.Vector3())?.x ?? 0).toFixed(3)),
      insideEnclosure: enclosureBounds
        ? enclosureBounds.containsPoint(
          new THREE.Vector3(
            entry.worldX ?? entry.mesh?.getWorldPosition?.(new THREE.Vector3())?.x ?? 0,
            entry.mesh?.getWorldPosition?.(new THREE.Vector3())?.y ?? 0,
            entry.mesh?.getWorldPosition?.(new THREE.Vector3())?.z ?? 0,
          ),
        )
        : null,
      centered: Boolean(entry.mesh?.userData?.bmsSpinCentered),
      spinCenter: entry.mesh?.userData?.bmsSpinCenter
        ? {
          x: Number(entry.mesh.userData.bmsSpinCenter.x.toFixed(3)),
          y: Number(entry.mesh.userData.bmsSpinCenter.y.toFixed(3)),
          z: Number(entry.mesh.userData.bmsSpinCenter.z.toFixed(3)),
        }
        : null,
    })),
    thresholds: lastModelSelectionDebug.thresholds,
  };

  updateConnectionVisualState(performance.now());
  scene.add(object);

  // Fit camera to object.
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

  // Save the actual launch position for view reset.
  DEFAULT_CAMERA_POSITION.copy(camera.position);
  DEFAULT_CAMERA_TARGET.copy(controls.target);

  updateConnectionVisualState(performance.now());
  setModelProcessProgress(MODEL_PROCESS_SEGMENTS.finalize.start, MODEL_PROCESS_SEGMENTS.finalize.span, 1);

  console.log(`Loaded model with ${cellMeshes.length} detected cells.`);
  console.log(`Detected ${shellMeshes.length} shell mesh(es), ${fanMeshes.length} fan mesh(es).`);

  // If no cells found, maybe log all names to help debug.
  if (cellMeshes.length === 0) {
    console.warn("No cells found matching pattern. Logging all mesh names:");
    object.traverse((child) => { if (child.isMesh) console.log(child.name); });

    // Fallback: Add a placeholder box so the user sees SOMETHING.
    const placeholder = new THREE.Mesh(
      new THREE.BoxGeometry(5, 5, 5),
      new THREE.MeshStandardMaterial({ color: 0xff0000, wireframe: true }),
    );
    scene.add(placeholder);

    // Add a text label if possible, or just log.
    const msg = document.createElement("div");
    msg.style.position = "absolute";
    msg.style.bottom = "20px";
    msg.style.left = "50%";
    msg.style.transform = "translateX(-50%)";
    msg.style.color = "orange";
    msg.innerText = "Warning: No battery cells found in model. Showing wireframe box.";
    document.body.appendChild(msg);
  }

  setBootDetail("3D model ready.");
  markBootModelReady();
}

async function loadEloadModel() {
  const gltfLoader = new GLTFLoader();

  return new Promise((resolve, reject) => {
    gltfLoader.load(
      ELOAD_MODEL_PATH,
      (gltf) => {
        const object = gltf.scene;

        // Wrap in a container group for proper centering
        const container = new THREE.Group();
        container.add(object);

        // Disable shadows for performance
        object.traverse((child) => {
          if (child.isMesh) {
            child.castShadow = false;
            child.receiveShadow = false;
          }
        });

        // Center the container based on bounds
        container.updateMatrixWorld(true);
        const bounds = new THREE.Box3().setFromObject(container);
        const center = bounds.getCenter(new THREE.Vector3());
        container.position.sub(center);

        // Add to the E-Load scene (not the BMS scene)
        eloadScene.add(container);
        loadedEloadModel = container;
        window.loadedEloadModel = container;  // Expose for debugging

        // Position camera to frame the model
        const size = bounds.getSize(new THREE.Vector3());
        const maxDim = Math.max(size.x, size.y, size.z);
        const fov = eloadCamera.fov * (Math.PI / 180);
        const cameraZ = Math.abs(maxDim / (2 * Math.tan(fov / 2))) * 2.0;

        // Default "hero" pose — low-angle exterior shot showing the solid model
        eloadCameraDefaultPos = new THREE.Vector3(cameraZ * 0.8, cameraZ * 0.25, cameraZ * 0.9);
        eloadCameraDefaultTarget = new THREE.Vector3(0, 0, 0);

        // Reveal "x-ray" pose — exact position from camera debug overlay
        eloadCameraRevealPos = new THREE.Vector3(0.322, 0.142, -0.261);
        eloadCameraRevealTarget = new THREE.Vector3(0.000, 0.006, 0.000);

        // Start at hero pose
        eloadCamera.position.copy(eloadCameraDefaultPos);
        eloadControls.maxDistance = cameraZ * 5;
        eloadControls.target.copy(eloadCameraDefaultTarget);
        eloadControls.update();

        // If the port was already connected before this tab was opened,
        // eloadHasRevealed will be true but the animation fired on null objects.
        // Reset progress to 0 and fire a fresh reveal now that the scene exists.
        if (eloadHasRevealed) {
          eloadRevealProgress = 0;
          eloadRevealFrom = 0;
          eloadRevealActive = false;
          startEloadReveal(true);
          console.log('[BMS] E-Load scene ready: replaying reveal (was pre-connected)');
        }

        console.log('[BMS] E-Load GLB model loaded, size:', size.x.toFixed(1), size.y.toFixed(1), size.z.toFixed(1));
        resolve(object);
      },
      (xhr) => {
        if (xhr.total > 0) {
          const percent = (xhr.loaded / xhr.total * 100).toFixed(1);
          console.log(`[E-Load] ${percent}% loaded`);
        }
      },
      (error) => {
        console.warn('[BMS] E-Load GLB model failed to load:', error);
        resolve(null);  // Graceful degradation
      }
    );
  });
}

async function initializeEloadModel(model) {
  if (!model) return;

  console.log('[BMS] Initializing E-Load model...');

  // Ensure environment map exists for reflections
  if (!eloadEnvMap) eloadEnvMap = buildStudioEnvMap();

  // Identify lid, shell, fan blade, and heatsink components
  model.traverse((child) => {
    if (!child?.isMesh) return;

    const name = (child.name || '').toLowerCase();
    const parentName = (child.parent?.name || '').toLowerCase();
    const combinedName = `${name} ${parentName}`;

    // Identify top lid for transparency
    if (ELOAD_LID_NAME_PATTERN.test(combinedName)) {
      eloadLidMeshes.push(child);
      console.log(`[BMS] E-Load lid identified: ${child.name}`);
    }

    // Identify shell/enclosure
    if (ELOAD_SHELL_NAME_PATTERN.test(combinedName)) {
      eloadShellMeshes.push(child);
      registerShellMaterialState(child);
    }

    // Identify fan blades - collect candidates for pivot setup below
    if (ELOAD_FAN_BLADE_NAME_PATTERN.test(name) || ELOAD_FAN_BLADE_NAME_PATTERN.test(parentName)) {
      const alreadyRegistered = eloadFanBladeMeshes.some(e => e.mesh === child);
      if (!alreadyRegistered) {
        eloadFanBladeMeshes.push({
          mesh: child,
          spinNode: null,  // Will be set up with pivot below
          axis: new THREE.Vector3(0, 1, 0),
        });
        console.log(`[BMS] E-Load fan blade candidate: ${child.name} (parent: ${child.parent?.name})`);
      }
    }

    // Identify heatsink
    if (ELOAD_HEATSINK_NAME_PATTERN.test(name) || ELOAD_HEATSINK_NAME_PATTERN.test(parentName)) {
      eloadHeatsinkMeshes.push(child);
      console.log(`[BMS] E-Load heatsink identified: ${child.name}`);
    }

    // Identify board / PCB
    if (PCB_NAME_PATTERN.test(combinedName)) {
      if (!eloadBoardMeshes.includes(child)) {
        eloadBoardMeshes.push(child);
        console.log(`[BMS] E-Load board identified: ${child.name}`);
      }
    }

    // Identify connectors / ports / terminals
    if (HARDWARE_NAME_PATTERN.test(combinedName)) {
      if (!eloadConnectorMeshes.includes(child)) {
        eloadConnectorMeshes.push(child);
      }
    }

    // Apply reflective material to ALL meshes
    applyReflectiveMaterial(child);
  });

  // Center each fan blade's geometry so it rotates around its own visual center.
  // No hierarchy changes — just shift the vertex buffer and compensate mesh.position.
  eloadFanBladeMeshes.forEach((entry) => {
    const mesh = entry.mesh;
    if (!mesh?.geometry) return;

    const geo = mesh.geometry;
    geo.computeBoundingBox();
    const geoCenter = geo.boundingBox.getCenter(new THREE.Vector3());
    const geoSize = geo.boundingBox.getSize(new THREE.Vector3());

    // Skip if already centered (or negligible offset)
    if (geoCenter.length() < 0.001) {
      entry.spinNode = mesh;
    } else {
      // Shift all vertices so the geometry center is at the mesh's local origin
      geo.translate(-geoCenter.x, -geoCenter.y, -geoCenter.z);

      // Compensate mesh position so the visual result stays the same.
      // geoCenter is in mesh-local space (pre-rotation/scale), so transform it
      // through the mesh's own rotation & scale to get the parent-space offset.
      const offset = geoCenter.clone();
      offset.multiply(mesh.scale);
      offset.applyQuaternion(mesh.quaternion);
      mesh.position.add(offset);

      entry.spinNode = mesh;
    }

    // Auto-detect spin axis from geometry bounds (thinnest dim = rotation axis)
    if (geoSize.x <= geoSize.y && geoSize.x <= geoSize.z) {
      entry.axis = new THREE.Vector3(1, 0, 0);
    } else if (geoSize.z <= geoSize.x && geoSize.z <= geoSize.y) {
      entry.axis = new THREE.Vector3(0, 0, 1);
    } else {
      entry.axis = new THREE.Vector3(0, 1, 0);
    }

    console.log(`[BMS] E-Load fan blade centered: ${mesh.name}, axis: [${entry.axis.x},${entry.axis.y},${entry.axis.z}], offset: [${geoCenter.x.toFixed(2)},${geoCenter.y.toFixed(2)},${geoCenter.z.toFixed(2)}]`);
  });

  // Set up thermal infrared camera visualization for heatsink / FET area
  setupThermalVisualization();

  // Store lid base opacity but keep fully solid initially (reveal transition will fade)
  eloadLidMeshes.forEach((mesh) => {
    toMaterialList(mesh).forEach((material) => {
      if (!material) return;
      material.userData.bmsEloadLidBaseOpacity = material.opacity ?? 1;
      // Start solid — the reveal transition will smoothly fade lid + shell later
    });
  });

  // Store shell base opacity (they also start solid)
  eloadShellMeshes.forEach((mesh) => {
    toMaterialList(mesh).forEach((material) => {
      if (!material) return;
      if (material.userData.bmsShellBaseOpacity === undefined) {
        material.userData.bmsShellBaseOpacity = material.opacity ?? 1;
      }
    });
  });

  console.log(`[BMS] E-Load initialized: ${eloadLidMeshes.length} lid, ${eloadFanBladeMeshes.length} fan blades, ${eloadHeatsinkMeshes.length} heatsink parts`);

  // Dump all mesh components for inspection
  const allParts = [];
  model.traverse((child) => {
    if (child.isMesh) {
      const matName = Array.isArray(child.material)
        ? child.material.map(m => m?.name || '(unnamed)').join(', ')
        : (child.material?.name || '(unnamed)');
      allParts.push({
        name: child.name || '(unnamed)',
        parent: child.parent?.name || '(root)',
        material: matName,
        vertices: child.geometry?.attributes?.position?.count || 0,
      });
    }
  });
  console.log(`[E-Load] All ${allParts.length} mesh components:`);
  console.table(allParts);
}

// --- E-Load Reflective Material ---
function buildStudioEnvMap() {
  // Create a simple gradient cube map for reflections
  const size = 256;
  const canvas = document.createElement('canvas');
  canvas.width = size;
  canvas.height = size;
  const ctx = canvas.getContext('2d');

  // Create a gradient that simulates a studio environment
  const gradient = ctx.createLinearGradient(0, 0, 0, size);
  gradient.addColorStop(0, '#667799');    // cool sky top
  gradient.addColorStop(0.3, '#8899aa');  // lighter mid
  gradient.addColorStop(0.5, '#aabbcc');  // horizon
  gradient.addColorStop(0.7, '#556677');  // floor reflection
  gradient.addColorStop(1, '#334455');    // dark bottom
  ctx.fillStyle = gradient;
  ctx.fillRect(0, 0, size, size);

  // Add subtle bright spots for specular highlights
  ctx.fillStyle = 'rgba(255, 255, 255, 0.15)';
  ctx.beginPath();
  ctx.arc(size * 0.3, size * 0.25, size * 0.12, 0, Math.PI * 2);
  ctx.fill();
  ctx.beginPath();
  ctx.arc(size * 0.7, size * 0.35, size * 0.08, 0, Math.PI * 2);
  ctx.fill();

  const texture = new THREE.CanvasTexture(canvas);
  texture.mapping = THREE.EquirectangularReflectionMapping;
  return texture;
}

function applyReflectiveMaterial(mesh) {
  if (!mesh?.isMesh || !eloadEnvMap) return;

  const materials = Array.isArray(mesh.material) ? mesh.material : [mesh.material];
  materials.forEach((mat) => {
    if (!mat) return;
    // Store originals
    mat.userData._origMetalness = mat.metalness;
    mat.userData._origRoughness = mat.roughness;

    // Make it reflective
    mat.envMap = eloadEnvMap;
    mat.envMapIntensity = 1.5;
    mat.metalness = Math.max(mat.metalness ?? 0, 0.6);
    mat.roughness = Math.min(mat.roughness ?? 1, 0.35);
    mat.needsUpdate = true;
  });
}

// --- E-Load Heat Visualization (Thermal Infrared Camera) ---

// GLSL vertex shader — transforms to world space for FET distance calculation
const THERMAL_VERT = `
varying vec3 vWorldPos;
varying vec3 vWorldNormal;

void main() {
  vec4 worldPos = modelMatrix * vec4(position, 1.0);
  vWorldPos = worldPos.xyz;
  vWorldNormal = normalize(mat3(modelMatrix) * normal);
  gl_Position = projectionMatrix * viewMatrix * worldPos;
}
`;

// GLSL fragment shader — lava-lamp style thermal visualization driven by 4 FET heat sources
const THERMAL_FRAG = `
precision highp float;

uniform float uTime;
uniform float uIntensity;
uniform vec3  uFetPos[4];
uniform float uFetHeat[4];
uniform float uHeatRadius;

varying vec3 vWorldPos;
varying vec3 vWorldNormal;

/* ---- value noise (hash-based 3D) ---- */
float hash(vec3 p) {
  p = fract(p * vec3(0.1031, 0.1030, 0.0973));
  p += dot(p, p.yxz + 33.33);
  return fract((p.x + p.y) * p.z);
}

float noise(vec3 p) {
  vec3 i = floor(p);
  vec3 f = fract(p);
  f = f * f * (3.0 - 2.0 * f);
  return mix(
    mix(mix(hash(i),                hash(i + vec3(1,0,0)), f.x),
        mix(hash(i + vec3(0,1,0)),  hash(i + vec3(1,1,0)), f.x), f.y),
    mix(mix(hash(i + vec3(0,0,1)),  hash(i + vec3(1,0,1)), f.x),
        mix(hash(i + vec3(0,1,1)),  hash(i + vec3(1,1,1)), f.x), f.y),
    f.z);
}

/* ---- lava lamp FBM: fewer octaves, larger scale for big flowing blobs ---- */
float lavaFbm(vec3 p) {
  float v = 0.0, a = 0.6;
  for (int i = 0; i < 3; i++) {
    v += a * noise(p);
    p = p * 1.8 + 0.3;
    a *= 0.45;
  }
  return v;
}

/* ---- thermal color ramp: dark-blue -> blue -> cyan -> green -> yellow -> orange -> red ---- */
vec3 thermalColor(float t) {
  t = clamp(t, 0.0, 1.0);
  if (t < 0.15) return mix(vec3(0.0, 0.0, 0.10), vec3(0.0, 0.0, 0.55), t / 0.15);
  if (t < 0.30) return mix(vec3(0.0, 0.0, 0.55), vec3(0.0, 0.45, 0.65), (t - 0.15) / 0.15);
  if (t < 0.45) return mix(vec3(0.0, 0.45, 0.65), vec3(0.0, 0.78, 0.22), (t - 0.30) / 0.15);
  if (t < 0.60) return mix(vec3(0.0, 0.78, 0.22), vec3(0.85, 0.85, 0.0), (t - 0.45) / 0.15);
  if (t < 0.78) return mix(vec3(0.85, 0.85, 0.0), vec3(1.0, 0.45, 0.0), (t - 0.60) / 0.18);
  return mix(vec3(1.0, 0.45, 0.0), vec3(0.95, 0.05, 0.0), (t - 0.78) / 0.22);
}

void main() {
  float temperature = 0.0;

  // Accumulate heat from each FET source with radial falloff
  for (int i = 0; i < 4; i++) {
    float dist = distance(vWorldPos, uFetPos[i]);
    float falloff = 1.0 - smoothstep(0.0, uHeatRadius, dist);
    falloff = falloff * falloff;
    temperature += uFetHeat[i] * falloff;
  }

  // === Lava lamp motion ===
  // Primary blob layer: large scale, strong upward flow
  vec3 p1 = vWorldPos * 1.5 + vec3(
    sin(uTime * 0.07) * 0.3,
    -uTime * 0.25,
    cos(uTime * 0.09) * 0.2
  );
  float lava1 = lavaFbm(p1);

  // Secondary blob layer: different scale and speed for organic variety
  vec3 p2 = vWorldPos * 2.2 + vec3(
    cos(uTime * 0.11) * 0.4,
    -uTime * 0.15,
    sin(uTime * 0.06) * 0.35
  );
  float lava2 = lavaFbm(p2);

  // Combine: main blobs + secondary detail
  float lavaPattern = lava1 * 0.65 + lava2 * 0.35;

  // Threshold to create distinct amorphous blob shapes
  lavaPattern = smoothstep(0.25, 0.65, lavaPattern);

  // Modulate temperature with lava pattern
  temperature += lavaPattern * 0.35 * uIntensity;

  // Slow breathing pulse (more dramatic than original)
  float pulse = 0.5 + 0.5 * sin(uTime * 0.8);
  temperature *= 0.85 + 0.15 * pulse;

  // Apply global intensity
  temperature *= uIntensity * 1.5;

  // Ambient minimum so cool areas stay dark-blue (not black)
  temperature = max(temperature, 0.04);

  vec3 color = thermalColor(temperature);
  gl_FragColor = vec4(color, 1.0);
}
`;

/**
 * Sets up the thermal infrared visualization system.
 * Identifies FET heat sources among heatsink meshes, computes world positions,
 * creates a shared ShaderMaterial, and registers meshes for material swapping.
 */
function setupThermalVisualization() {
  if (eloadHeatsinkMeshes.length === 0) return;

  // Ensure world matrices are current before reading positions
  if (loadedEloadModel) loadedEloadModel.updateMatrixWorld(true);

  // Identify FET meshes among heatsink meshes — these are the heat sources
  const fetMeshes = eloadHeatsinkMeshes.filter(
    (m) => ELOAD_FET_NAME_PATTERN.test(m.name)
  );

  // Compute world-space centers for each FET
  eloadFetWorldPositions = [];
  fetMeshes.forEach((m) => {
    m.geometry.computeBoundingBox();
    const center = new THREE.Vector3();
    m.geometry.boundingBox.getCenter(center);
    m.localToWorld(center);
    eloadFetWorldPositions.push(center);
    console.log(
      `[BMS] FET heat source: ${m.name} at (${center.x.toFixed(2)}, ${center.y.toFixed(2)}, ${center.z.toFixed(2)})`
    );
  });

  // Fallback — if no FET meshes found, use centers of first heatsink meshes
  if (eloadFetWorldPositions.length === 0) {
    console.warn("[BMS] No FET meshes found by name; using heatsink centers as heat sources");
    eloadHeatsinkMeshes.slice(0, 4).forEach((m) => {
      m.geometry.computeBoundingBox();
      const c = new THREE.Vector3();
      m.geometry.boundingBox.getCenter(c);
      m.localToWorld(c);
      eloadFetWorldPositions.push(c);
    });
  }

  // Pad to exactly 4 (unused slots have heat = 0 so they contribute nothing)
  while (eloadFetWorldPositions.length < 4) {
    eloadFetWorldPositions.push(
      eloadFetWorldPositions.length > 0
        ? eloadFetWorldPositions[eloadFetWorldPositions.length - 1].clone()
        : new THREE.Vector3()
    );
  }
  eloadFetWorldPositions = eloadFetWorldPositions.slice(0, 4);

  // Initialize per-FET heat levels (slider default until telemetry arrives)
  eloadFetHeatLevels = new Array(4).fill(0);
  for (let i = 0; i < Math.min(fetMeshes.length, 4); i++) eloadFetHeatLevels[i] = 0.5;

  // Compute heat radius from model extents
  if (loadedEloadModel) {
    const modelBox = new THREE.Box3().setFromObject(loadedEloadModel);
    const sz = modelBox.getSize(new THREE.Vector3());
    eloadThermalHeatRadius = Math.max(sz.x, sz.y, sz.z) * 0.35;
  }

  // Create shared thermal ShaderMaterial
  eloadThermalShaderMat = new THREE.ShaderMaterial({
    uniforms: {
      uTime: { value: 0.0 },
      uIntensity: { value: eloadHeatIntensity },
      uFetPos: { value: eloadFetWorldPositions },
      uFetHeat: { value: eloadFetHeatLevels },
      uHeatRadius: { value: eloadThermalHeatRadius },
    },
    vertexShader: THERMAL_VERT,
    fragmentShader: THERMAL_FRAG,
    side: THREE.DoubleSide,
  });

  // Register all heatsink meshes — store original material for toggling
  eloadHeatsinkMeshes.forEach((mesh) => {
    const origMat = Array.isArray(mesh.material) ? [...mesh.material] : mesh.material;
    eloadThermalEntries.push({ mesh, originalMat: origMat });
    if (eloadHeatVizEnabled) {
      mesh.material = eloadThermalShaderMat;
    }
    console.log(`[BMS] Thermal viz registered: ${mesh.name}`);
  });

  console.log(
    `[BMS] Thermal visualization ready — ${fetMeshes.length} FET sources, ` +
    `${eloadHeatsinkMeshes.length} thermal meshes, radius: ${eloadThermalHeatRadius.toFixed(2)}`
  );
}

function updateEloadHeatVisualization(delta) {
  if (!eloadThermalShaderMat || !eloadHeatVizEnabled) return;

  eloadHeatClock += delta;
  eloadThermalShaderMat.uniforms.uTime.value = eloadHeatClock;
  eloadThermalShaderMat.uniforms.uIntensity.value = eloadHeatIntensity;

  // Per-FET heat levels are updated live by updateEloadTelemetry()
  for (let i = 0; i < 4; i++) {
    eloadThermalShaderMat.uniforms.uFetHeat.value[i] = eloadFetHeatLevels[i];
  }
}

// --- E-Load Reveal Transition (solid → transparent + camera orbit) ---

function startEloadReveal(reveal) {
  const target = reveal ? 1 : 0;
  // Skip if already there
  if (
    !eloadRevealActive &&
    Math.abs(eloadRevealProgress - target) < 1e-4 &&
    Math.abs(eloadRevealTo - target) < 1e-4
  ) {
    return;
  }

  eloadRevealFrom = eloadRevealProgress;
  eloadRevealTo = target;
  eloadRevealStartMs = performance.now();
  eloadRevealActive = true;

  // Snapshot current camera position as "from" for smooth interpolation
  if (eloadCamera) {
    eloadCameraFromPos = eloadCamera.position.clone();
    eloadCameraFromTarget = eloadControls ? eloadControls.target.clone() : new THREE.Vector3();
  }

  console.log(`[BMS] E-Load reveal transition: ${eloadRevealFrom.toFixed(2)} → ${target}`);
}

function updateEloadRevealTransition(nowMs) {
  if (!eloadRevealActive) return;

  const elapsed = Math.max(0, nowMs - eloadRevealStartMs);
  const t = Math.min(1, elapsed / ELOAD_REVEAL_MS);
  const eased = easeInOutCubic(t);

  eloadRevealProgress = THREE.MathUtils.lerp(eloadRevealFrom, eloadRevealTo, eased);

  if (t >= 1) {
    eloadRevealProgress = eloadRevealTo;
    eloadRevealActive = false;
  }

  // Camera uses eased (0→1) so it always goes snapshot → destination
  applyEloadRevealCamera(eased);
  // Transparency uses absolute progress (0 = solid, 1 = transparent)
  applyEloadRevealTransparency(eloadRevealProgress);
}

function applyEloadRevealCamera(animT) {
  if (!eloadCamera || !eloadControls) return;
  if (!eloadCameraDefaultPos || !eloadCameraRevealPos) return;

  const b = THREE.MathUtils.clamp(animT, 0, 1);

  // Snapshot of where the camera was when the transition started
  const fromPos = eloadCameraFromPos || eloadCameraDefaultPos;
  const fromTarget = eloadCameraFromTarget || eloadCameraDefaultTarget;

  // Destination depends on direction
  const destPos = eloadRevealTo >= 0.5 ? eloadCameraRevealPos : eloadCameraDefaultPos;
  const destTarget = eloadRevealTo >= 0.5 ? eloadCameraRevealTarget : eloadCameraDefaultTarget;

  // b goes 0→1: snapshot → destination (works for both directions)
  eloadCamera.position.copy(fromPos).lerp(destPos, b);
  eloadControls.target.copy(fromTarget).lerp(destTarget, b);
}

function applyEloadRevealTransparency(blend) {
  const b = THREE.MathUtils.clamp(blend, 0, 1);

  // Shell: solid at b=0 → semi-transparent at b=1
  eloadShellMeshes.forEach((mesh) => {
    toMaterialList(mesh).forEach((material) => {
      if (!material) return;
      const baseOpacity = material.userData.bmsShellBaseOpacity ?? 1;
      const targetOpacity = THREE.MathUtils.lerp(baseOpacity, 0.18, b);
      material.transparent = targetOpacity < 0.99;
      material.depthWrite = targetOpacity >= 0.99;
      material.opacity = targetOpacity;
      material.needsUpdate = true;
    });
  });

  // Lid: solid at b=0 → very transparent at b=1
  eloadLidMeshes.forEach((mesh) => {
    toMaterialList(mesh).forEach((material) => {
      if (!material) return;
      const baseOpacity = material.userData.bmsEloadLidBaseOpacity ?? 1;
      const targetOpacity = THREE.MathUtils.lerp(baseOpacity, 0.15, b);
      material.transparent = targetOpacity < 0.99;
      material.depthWrite = targetOpacity >= 0.99;
      material.opacity = targetOpacity;
      material.needsUpdate = true;
    });
  });
}

// --- E-Load Fan Animation ---
function updateEloadFanSpin(delta) {
  if (!eloadFanSpinEnabled || eloadFanBladeMeshes.length === 0) return;

  const spinSpeed = THREE.MathUtils.lerp(ELOAD_FAN_SPIN_BASE, ELOAD_FAN_SPIN_MAX, eloadFanSpinSpeed);
  const rotationDelta = delta * spinSpeed;

  eloadFanBladeMeshes.forEach((entry, idx) => {
    if (!entry.spinNode) return;
    const direction = idx % 2 === 0 ? 1 : -1;
    entry.spinNode.rotateOnAxis(entry.axis, direction * rotationDelta);
  });
}

function handleModelLoadFailure(error) {
  console.error("An error happened loading the FBX:", error);
  const message = (error && typeof error.message === "string" && error.message.trim().length > 0)
    ? `3D model loading error: ${error.message}`
    : "3D model loading error.";
  setBootError(message, error);
}

setBootStageProgress("bootstrap", 0.85);
setBootStage("modelDownload", "Loading 3D model...");
setBootDetail("Waiting for transfer...");
console.log("Starting FBX load...");
const loader = new FBXLoader();
loader.load(
  MODEL_PATH,
  (object) => {
    void initializeLoadedModel(object).catch(handleModelLoadFailure);
  },
  (xhr) => {
    const loaded = Number(xhr?.loaded) || 0;
    const total = Number(xhr?.total);
    setBootStage("modelDownload", "Loading 3D model...");
    updateBootDownloadProgress(loaded, total);
    if (Number.isFinite(total) && total > 0) {
      console.log(`${((loaded / total) * 100).toFixed(1)}% loaded`);
    } else {
      console.log(`${formatBootBytes(loaded)} downloaded`);
    }
  },
  (error) => {
    handleModelLoadFailure(error);
  },
);

window.__bmsDebugModel = function __bmsDebugModel() {
  return {
    ...lastModelSelectionDebug,
    cellCount: cellMeshes.length,
    shellCount: shellMeshes.length,
    fanCount: fanMeshes.length,
    selectedCellUuids: [...selectedCellUuidSet.values()],
  };
};

window.__bmsDumpCellSelection = function __bmsDumpCellSelection() {
  const rows = cellMeshes.map((entry) => {
    const info = meshInfos.find((item) => item.uuid === entry.mesh?.uuid);
    return {
      id: entry.id,
      name: entry.mesh?.name || "",
      parent: entry.mesh?.parent?.name || "",
      uuid: entry.mesh?.uuid || "",
      x: info ? Number(info.center.x.toFixed(3)) : null,
      y: info ? Number(info.center.y.toFixed(3)) : null,
      z: info ? Number(info.center.z.toFixed(3)) : null,
    };
  });
  console.table(rows);
  console.log("[BMS] Model debug", window.__bmsDebugModel());
  return rows;
};

function connectedPoseSnapshot() {
  return {
    transitionMs: CONNECTION_TRANSITION_MS,
    positionOffset: {
      x: Number(CONNECTED_MODEL_POSITION_OFFSET.x.toFixed(4)),
      y: Number(CONNECTED_MODEL_POSITION_OFFSET.y.toFixed(4)),
      z: Number(CONNECTED_MODEL_POSITION_OFFSET.z.toFixed(4)),
    },
    rotationOffsetRad: {
      x: Number(CONNECTED_MODEL_ROTATION_OFFSET.x.toFixed(5)),
      y: Number(CONNECTED_MODEL_ROTATION_OFFSET.y.toFixed(5)),
      z: Number(CONNECTED_MODEL_ROTATION_OFFSET.z.toFixed(5)),
    },
    rotationOffsetDeg: {
      x: Number(THREE.MathUtils.radToDeg(CONNECTED_MODEL_ROTATION_OFFSET.x).toFixed(3)),
      y: Number(THREE.MathUtils.radToDeg(CONNECTED_MODEL_ROTATION_OFFSET.y).toFixed(3)),
      z: Number(THREE.MathUtils.radToDeg(CONNECTED_MODEL_ROTATION_OFFSET.z).toFixed(3)),
    },
    visualProgress: Number(connectionVisualProgress.toFixed(3)),
  };
}

function refreshConnectedPoseFromCurrentOffsets() {
  if (!loadedModel) return connectedPoseSnapshot();
  configureModelPoseTargets(loadedModel);
  applyModelConnectionPose(connectionVisualProgress);
  return connectedPoseSnapshot();
}

window.__bmsGetConnectedPose = function __bmsGetConnectedPose() {
  return connectedPoseSnapshot();
};

window.__bmsSetConnectedPose = function __bmsSetConnectedPose({
  position,
  rotationDeg,
  rotationRad,
  transitionMs,
} = {}) {
  if (position && typeof position === "object") {
    if (Number.isFinite(position.x)) CONNECTED_MODEL_POSITION_OFFSET.x = position.x;
    if (Number.isFinite(position.y)) CONNECTED_MODEL_POSITION_OFFSET.y = position.y;
    if (Number.isFinite(position.z)) CONNECTED_MODEL_POSITION_OFFSET.z = position.z;
  }

  if (rotationRad && typeof rotationRad === "object") {
    if (Number.isFinite(rotationRad.x)) CONNECTED_MODEL_ROTATION_OFFSET.x = rotationRad.x;
    if (Number.isFinite(rotationRad.y)) CONNECTED_MODEL_ROTATION_OFFSET.y = rotationRad.y;
    if (Number.isFinite(rotationRad.z)) CONNECTED_MODEL_ROTATION_OFFSET.z = rotationRad.z;
  }

  if (rotationDeg && typeof rotationDeg === "object") {
    if (Number.isFinite(rotationDeg.x)) {
      CONNECTED_MODEL_ROTATION_OFFSET.x = THREE.MathUtils.degToRad(rotationDeg.x);
    }
    if (Number.isFinite(rotationDeg.y)) {
      CONNECTED_MODEL_ROTATION_OFFSET.y = THREE.MathUtils.degToRad(rotationDeg.y);
    }
    if (Number.isFinite(rotationDeg.z)) {
      CONNECTED_MODEL_ROTATION_OFFSET.z = THREE.MathUtils.degToRad(rotationDeg.z);
    }
  }

  if (Number.isFinite(transitionMs) && transitionMs >= 0) {
    CONNECTION_TRANSITION_MS = transitionMs;
  }

  return refreshConnectedPoseFromCurrentOffsets();
};

window.__bmsNudgeConnectedPose = function __bmsNudgeConnectedPose({
  dx = 0,
  dy = 0,
  dz = 0,
  drxDeg = 0,
  dryDeg = 0,
  drzDeg = 0,
} = {}) {
  if (Number.isFinite(dx)) CONNECTED_MODEL_POSITION_OFFSET.x += dx;
  if (Number.isFinite(dy)) CONNECTED_MODEL_POSITION_OFFSET.y += dy;
  if (Number.isFinite(dz)) CONNECTED_MODEL_POSITION_OFFSET.z += dz;
  if (Number.isFinite(drxDeg)) CONNECTED_MODEL_ROTATION_OFFSET.x += THREE.MathUtils.degToRad(drxDeg);
  if (Number.isFinite(dryDeg)) CONNECTED_MODEL_ROTATION_OFFSET.y += THREE.MathUtils.degToRad(dryDeg);
  if (Number.isFinite(drzDeg)) CONNECTED_MODEL_ROTATION_OFFSET.z += THREE.MathUtils.degToRad(drzDeg);

  return refreshConnectedPoseFromCurrentOffsets();
};

window.__bmsPreviewConnectedPose = function __bmsPreviewConnectedPose(enabled = true) {
  const target = enabled ? 1 : 0;
  connectionVisualProgress = target;
  connectionTransitionFrom = target;
  connectionTransitionTo = target;
  connectionTransitionActive = false;
  applyShellTransparency(connectionVisualProgress);
  applyModelConnectionPose(connectionVisualProgress);
  return connectedPoseSnapshot();
};

window.__bmsGetViewState = function __bmsGetViewState() {
  const azimuth = typeof controls.getAzimuthalAngle === "function"
    ? controls.getAzimuthalAngle()
    : 0;
  const polar = typeof controls.getPolarAngle === "function"
    ? controls.getPolarAngle()
    : 0;
  return {
    cameraPosition: {
      x: Number(camera.position.x.toFixed(4)),
      y: Number(camera.position.y.toFixed(4)),
      z: Number(camera.position.z.toFixed(4)),
    },
    cameraUp: {
      x: Number(camera.up.x.toFixed(4)),
      y: Number(camera.up.y.toFixed(4)),
      z: Number(camera.up.z.toFixed(4)),
    },
    target: {
      x: Number(controls.target.x.toFixed(4)),
      y: Number(controls.target.y.toFixed(4)),
      z: Number(controls.target.z.toFixed(4)),
    },
    orbitAnglesDeg: {
      azimuth: Number(THREE.MathUtils.radToDeg(azimuth).toFixed(3)),
      polar: Number(THREE.MathUtils.radToDeg(polar).toFixed(3)),
    },
  };
};

window.__bmsResetView = function __bmsResetView() {
  camera.position.copy(DEFAULT_CAMERA_POSITION);
  camera.up.set(0, 1, 0);
  controls.target.copy(DEFAULT_CAMERA_TARGET);
  controls.update();
  return window.__bmsGetViewState();
};

function cameraQuaternionFromPose(position, target, up) {
  const probe = new THREE.Object3D();
  probe.position.copy(position);
  probe.up.copy(up || new THREE.Vector3(0, 1, 0));
  probe.lookAt(target);
  probe.updateMatrixWorld(true);
  return probe.quaternion.clone();
}

window.__bmsCaptureViewAsConnectedPose = function __bmsCaptureViewAsConnectedPose({
  apply = false,
  resetView = false,
  previewConnected = false,
  transitionMs,
} = {}) {
  if (!loadedModel) {
    return { error: "Model is not loaded yet." };
  }

  const defaultCamQ = cameraQuaternionFromPose(
    DEFAULT_CAMERA_POSITION.clone(),
    DEFAULT_CAMERA_TARGET.clone(),
    new THREE.Vector3(0, 1, 0),
  );
  const currentCamQ = cameraQuaternionFromPose(
    camera.position.clone(),
    controls.target.clone(),
    camera.up.clone(),
  );

  // Camera relative rotation from default -> current.
  const relativeCamQ = currentCamQ.clone().multiply(defaultCamQ.clone().invert());
  // Equivalent model rotation is inverse of that camera relative rotation.
  const modelOffsetQ = relativeCamQ.clone().invert();
  const capturedEuler = new THREE.Euler().setFromQuaternion(modelOffsetQ, "XYZ");

  const viewStateBeforeReset = window.__bmsGetViewState();
  const capturedPose = {
    transitionMs: Number(
      (Number.isFinite(transitionMs) && transitionMs >= 0)
        ? transitionMs
        : CONNECTION_TRANSITION_MS
    ),
    positionOffset: {
      x: Number(CONNECTED_MODEL_POSITION_OFFSET.x.toFixed(4)),
      y: Number(CONNECTED_MODEL_POSITION_OFFSET.y.toFixed(4)),
      z: Number(CONNECTED_MODEL_POSITION_OFFSET.z.toFixed(4)),
    },
    rotationOffsetRad: {
      x: Number(capturedEuler.x.toFixed(5)),
      y: Number(capturedEuler.y.toFixed(5)),
      z: Number(capturedEuler.z.toFixed(5)),
    },
    rotationOffsetDeg: {
      x: Number(THREE.MathUtils.radToDeg(capturedEuler.x).toFixed(3)),
      y: Number(THREE.MathUtils.radToDeg(capturedEuler.y).toFixed(3)),
      z: Number(THREE.MathUtils.radToDeg(capturedEuler.z).toFixed(3)),
    },
    sourceView: viewStateBeforeReset,
  };

  if (!apply) {
    return capturedPose;
  }

  if (Number.isFinite(transitionMs) && transitionMs >= 0) {
    CONNECTION_TRANSITION_MS = transitionMs;
  }
  CONNECTED_MODEL_ROTATION_OFFSET.set(capturedEuler.x, capturedEuler.y, capturedEuler.z, "XYZ");
  const appliedPose = refreshConnectedPoseFromCurrentOffsets();

  if (resetView) {
    window.__bmsResetView();
  }
  if (previewConnected) {
    window.__bmsPreviewConnectedPose(true);
  }

  return {
    ...appliedPose,
    sourceView: viewStateBeforeReset,
  };
};

// single-underscore aliases for view helpers
window._bmsGetViewState = window.__bmsGetViewState;
window._bmsResetView = window.__bmsResetView;
window._bmsCaptureViewAsConnectedPose = window.__bmsCaptureViewAsConnectedPose;

// Backward-compatible aliases (single underscore typo-safe commands).
window._bmsGetConnectedPose = window.__bmsGetConnectedPose;
window._bmsSetConnectedPose = window.__bmsSetConnectedPose;
window._bmsNudgeConnectedPose = window.__bmsNudgeConnectedPose;
window._bmsPreviewConnectedPose = window.__bmsPreviewConnectedPose;

// --- Interaction (Raycaster + Click-to-Select Popup) ---
const raycaster = new THREE.Raycaster();
const mouse = new THREE.Vector2();
const eloadRaycaster = new THREE.Raycaster();
const eloadMouse = new THREE.Vector2();

const partPopupEl = document.getElementById("hover-tooltip");
const partPopupTitle = partPopupEl?.querySelector(".hover-tooltip__title");
const partPopupBody = partPopupEl?.querySelector(".hover-tooltip__body");

let selectedPart = null;           // { type, id?, fanId?, meshes: Mesh[], anchorMesh: Mesh }
let selectedPartKey = null;        // string key for toggle-off detection
let partPopupVisible = false;
let partPopupDataKey = null;       // cache key to avoid redundant DOM writes
const HIGHLIGHT_COLOR = 0x0a84ff;
const HIGHLIGHT_INTENSITY = 0.5;

// --- Mesh identification ---

// Walk up the scene graph from a mesh to see if any ancestor is a classified object.
function findAncestorFanEntry(obj) {
  let node = obj?.parent;
  while (node && node !== scene) {
    const entry = fanMeshes.find(e => e.spinNode === node || e.mesh === node);
    if (entry) return entry;
    node = node.parent;
  }
  return null;
}

function findAncestorEloadFan(obj) {
  let node = obj?.parent;
  while (node && node !== eloadScene) {
    const entry = eloadFanBladeMeshes.find(e => e.spinNode === node || e.mesh === node);
    if (entry) return entry;
    node = node.parent;
  }
  return null;
}

function identifyBmsClickTarget(intersects) {
  for (const hit of intersects) {
    const obj = hit.object;

    // 1. Direct cell match?
    const cellEntry = cellMeshes.find(e => e.mesh === obj);
    if (cellEntry) return { type: "bms-cell", id: cellEntry.id, meshes: [cellEntry.mesh], anchorMesh: cellEntry.mesh };

    // 2. Direct or descendant fan match?
    let fanEntry = fanMeshes.find(e => {
      if (!e?.mesh) return false;
      if (e.mesh === obj) return true;
      if (e.spinNode) {
        let match = false;
        e.spinNode.traverse(c => { if (c === obj) match = true; });
        return match;
      }
      return false;
    });
    // 2b. Ancestor walk: the clicked mesh may be a child of a fan assembly
    if (!fanEntry) fanEntry = findAncestorFanEntry(obj);
    if (fanEntry) {
      const allFanMeshes = [];
      if (fanEntry.spinNode) fanEntry.spinNode.traverse(c => { if (c.isMesh) allFanMeshes.push(c); });
      else if (fanEntry.mesh) allFanMeshes.push(fanEntry.mesh);
      return { type: "bms-fan", fanId: fanEntry.fanId || 1, meshes: allFanMeshes, anchorMesh: fanEntry.mesh };
    }

    // 3. Board / PCB
    if (boardMeshes.includes(obj)) {
      return { type: "bms-board", meshes: [obj], anchorMesh: obj };
    }

    // 4. Connector / port / terminal
    if (connectorMeshes.includes(obj)) {
      return { type: "bms-connector", meshes: [obj], anchorMesh: obj };
    }

    // 5. Shell match — highlight only the clicked shell piece, not all shells
    const shellEntry = shellMeshes.find(e => e.mesh === obj);
    if (shellEntry) {
      return { type: "bms-shell", meshes: [obj], anchorMesh: obj };
    }
  }
  return null;
}

function identifyEloadClickTarget(intersects) {
  for (const hit of intersects) {
    const obj = hit.object;

    // 1. Direct heatsink match
    if (eloadHeatsinkMeshes.includes(obj)) {
      return { type: "eload-heatsink", meshes: [obj], anchorMesh: obj };
    }

    // 2. Direct fan blade match
    const fanBlade = eloadFanBladeMeshes.find(e => e.mesh === obj);
    if (fanBlade) {
      return { type: "eload-fan", meshes: [fanBlade.mesh], anchorMesh: fanBlade.mesh };
    }
    // 2b. Ancestor walk for fan children
    const ancestorFan = findAncestorEloadFan(obj);
    if (ancestorFan) {
      return { type: "eload-fan", meshes: [ancestorFan.mesh], anchorMesh: ancestorFan.mesh };
    }

    // 3. Board / PCB
    if (eloadBoardMeshes.includes(obj)) {
      return { type: "eload-board", meshes: [obj], anchorMesh: obj };
    }

    // 4. Connector / port / terminal
    if (eloadConnectorMeshes.includes(obj)) {
      return { type: "eload-connector", meshes: [obj], anchorMesh: obj };
    }

    // 5. Lid — individual piece
    if (eloadLidMeshes.includes(obj)) {
      return { type: "eload-lid", meshes: [obj], anchorMesh: obj };
    }

    // 6. Shell — individual piece
    if (eloadShellMeshes.includes(obj)) {
      return { type: "eload-shell", meshes: [obj], anchorMesh: obj };
    }
  }
  return null;
}

function partKey(target) {
  if (!target) return null;
  if (target.type === "bms-cell") return `bms-cell-${target.id}`;
  if (target.type === "bms-fan") return `bms-fan-${target.fanId}`;
  // For board/connector, use the mesh uuid so each piece is independently toggleable
  if (target.type === "bms-board" || target.type === "bms-connector" ||
    target.type === "eload-board" || target.type === "eload-connector") {
    return `${target.type}-${target.anchorMesh?.uuid || ''}`;
  }
  return target.type;
}

// --- Highlight / Unhighlight ---
function applyHighlight(meshes) {
  meshes.forEach(mesh => {
    if (!mesh?.isMesh) return;
    const mats = Array.isArray(mesh.material) ? mesh.material : [mesh.material];
    mats.forEach(mat => {
      if (!mat) return;
      // Save original emissive state
      if (mat.userData._origEmissiveHex === undefined && mat.emissive) {
        mat.userData._origEmissiveHex = mat.emissive.getHex();
        mat.userData._origEmissiveIntensity = mat.emissiveIntensity;
      }
      if (mat.emissive) {
        mat.emissive.setHex(HIGHLIGHT_COLOR);
        mat.emissiveIntensity = HIGHLIGHT_INTENSITY;
      }
    });
  });
}

function removeHighlight(meshes) {
  meshes.forEach(mesh => {
    if (!mesh?.isMesh) return;
    const mats = Array.isArray(mesh.material) ? mesh.material : [mesh.material];
    mats.forEach(mat => {
      if (!mat || !mat.emissive) return;
      if (mat.userData._origEmissiveHex !== undefined) {
        mat.emissive.setHex(mat.userData._origEmissiveHex);
        mat.emissiveIntensity = mat.userData._origEmissiveIntensity || 0;
        delete mat.userData._origEmissiveHex;
        delete mat.userData._origEmissiveIntensity;
      } else {
        mat.emissive.setHex(0x000000);
        mat.emissiveIntensity = 0;
      }
    });
  });
}

// --- Popup content ---
function buildPartPopupContent(target) {
  if (!target) return null;
  switch (target.type) {
    case "bms-cell": {
      const cell = currentState?.cells?.find(c => c.id === target.id);
      const validCells = (currentState?.cells || []).filter(c => isFiniteNumber(c?.voltage));
      const avgV = validCells.length ? validCells.reduce((s, c) => s + c.voltage, 0) / validCells.length : null;
      const delta = cell && isFiniteNumber(cell.voltage) && avgV !== null
        ? ((cell.voltage - avgV) * 1000).toFixed(1) : null;
      return {
        title: `Cell ${String(target.id).padStart(2, "0")}`,
        rows: [
          { label: "Voltage", value: cell && isFiniteNumber(cell.voltage) ? `${cell.voltage.toFixed(3)} V` : "-- V" },
          { label: "Temperature", value: cell && isFiniteNumber(cell.temperature) && cell.temperature > -200 ? `${cell.temperature.toFixed(1)} \u00B0C` : "N/A" },
          { label: "\u0394 Avg", value: delta !== null ? `${(delta > 0 ? "+" : "") + delta} mV` : "-- mV" },
        ],
      };
    }
    case "bms-fan": {
      const fanData = target.fanId === 2 ? currentState?.fan2 : currentState?.fan1;
      const rpm = parseRpmValue(fanData?.rpm) ?? 0;
      const fc = currentState?.fan_control || {};
      return {
        title: `Fan ${target.fanId}`,
        rows: [
          { label: "Speed", value: rpm > 0 ? `${Math.round(rpm).toLocaleString()} RPM` : "-- RPM" },
          { label: "Mode", value: fc.auto ? "Auto" : "Manual" },
          { label: "Duty", value: isFiniteNumber(fc.duty) ? `${fc.duty}%` : "-- %" },
        ],
      };
    }
    case "bms-shell": {
      const validCells = (currentState?.cells || []).filter(c => isFiniteNumber(c?.voltage));
      const totalV = validCells.reduce((s, c) => s + c.voltage, 0);
      const packI = currentState?.pack_current;
      return {
        title: "Battery Pack",
        rows: [
          { label: "Pack Voltage", value: validCells.length ? `${totalV.toFixed(2)} V` : "-- V" },
          { label: "Pack Current", value: isFiniteNumber(packI) ? `${packI.toFixed(3)} A` : "-- A" },
          { label: "Active Cells", value: `${validCells.length} / ${CELL_COUNT}` },
        ],
      };
    }
    case "eload-heatsink": {
      const eload = currentState?.eload || {};
      const avgHeat = eloadFetHeatLevels.reduce((a, b) => a + b, 0) / eloadFetHeatLevels.length;
      return {
        title: "Heatsink / FETs",
        rows: [
          { label: "I_SET", value: isFiniteNumber(eload.i_set) ? `${eload.i_set.toFixed(1)} mV` : "-- mV" },
          { label: "Heat Level", value: `${(avgHeat * 100).toFixed(0)}%` },
          { label: "FET Count", value: `${eloadFetWorldPositions.length}` },
        ],
      };
    }
    case "eload-fan": {
      return {
        title: "Cooling Fan",
        rows: [
          { label: "Spin", value: eloadFanSpinEnabled ? "Active" : "Off" },
          { label: "Speed", value: `${Math.round(eloadFanSpinSpeed * 100)}%` },
        ],
      };
    }
    case "eload-shell":
    case "eload-lid": {
      const eload = currentState?.eload || {};
      return {
        title: target.type === "eload-lid" ? "E-Load Lid" : "E-Load Enclosure",
        rows: [
          { label: "VSENSE", value: isFiniteNumber(eload.v) ? `${(eload.v * 1000).toFixed(0)} mV` : "-- mV" },
          { label: "I_SET", value: isFiniteNumber(eload.i_set) ? `${eload.i_set.toFixed(1)} mV` : "-- mV" },
          { label: "DAC", value: isFiniteNumber(eload.dac) ? `${Math.round(eload.dac)}` : "--" },
        ],
      };
    }
    case "bms-board": {
      const validCells = (currentState?.cells || []).filter(c => isFiniteNumber(c?.voltage));
      const totalV = validCells.reduce((s, c) => s + c.voltage, 0);
      return {
        title: "BMS Controller Board",
        rows: [
          { label: "Pack Voltage", value: validCells.length ? `${totalV.toFixed(2)} V` : "-- V" },
          { label: "Active Cells", value: `${validCells.length} / ${CELL_COUNT}` },
          { label: "Status", value: backendConnectionState ? "Online" : "Offline" },
        ],
      };
    }
    case "bms-connector": {
      const meshName = target.anchorMesh?.name || target.anchorMesh?.parent?.name || "Component";
      // Clean up the mesh name for display
      const displayName = meshName.replace(/[_-]/g, ' ').replace(/\b\w/g, c => c.toUpperCase()).trim() || "Connector";
      return {
        title: displayName,
        rows: [
          { label: "Type", value: "Port / Connector" },
          { label: "Link", value: backendConnectionState ? "Active" : "Idle" },
        ],
      };
    }
    case "eload-board": {
      const eload = currentState?.eload || {};
      return {
        title: "E-Load Controller",
        rows: [
          { label: "VSENSE", value: isFiniteNumber(eload.v) ? `${(eload.v * 1000).toFixed(0)} mV` : "-- mV" },
          { label: "DAC", value: isFiniteNumber(eload.dac) ? `${Math.round(eload.dac)}` : "--" },
          { label: "Status", value: eload.enabled ? "Active" : "Standby" },
        ],
      };
    }
    case "eload-connector": {
      const meshName = target.anchorMesh?.name || target.anchorMesh?.parent?.name || "Component";
      const displayName = meshName.replace(/[_-]/g, ' ').replace(/\b\w/g, c => c.toUpperCase()).trim() || "Connector";
      const eload = currentState?.eload || {};
      return {
        title: displayName,
        rows: [
          { label: "Type", value: "Port / Terminal" },
          { label: "E-Load", value: eload.enabled ? "Enabled" : "Disabled" },
        ],
      };
    }
    default: return null;
  }
}

// --- Popup show / hide / position ---
function showPartPopup(content) {
  if (!content || !partPopupEl) return;
  partPopupDataKey = null; // force first write
  updatePartPopupContent(content);
  partPopupVisible = true;
  partPopupEl.classList.add("is-visible");
}

function updatePartPopupContent(content) {
  if (!content || !partPopupEl) return;
  const cacheKey = content.title + content.rows.map(r => r.value).join("|");
  if (cacheKey === partPopupDataKey) return;
  partPopupDataKey = cacheKey;
  partPopupTitle.textContent = content.title;
  partPopupBody.innerHTML = content.rows.map(r =>
    `<div class="hover-tooltip__row"><span class="hover-tooltip__label">${r.label}</span><span class="hover-tooltip__value">${r.value}</span></div>`
  ).join("");
}

function hidePartPopup() {
  if (partPopupVisible) {
    partPopupVisible = false;
    partPopupDataKey = null;
    partPopupEl.classList.remove("is-visible");
  }
}

function positionPopupAtMesh(mesh, cam, rendererDom) {
  if (!partPopupEl || !mesh || !cam || !rendererDom) return;
  // Get mesh world center
  const box = new THREE.Box3().setFromObject(mesh);
  const center = box.getCenter(new THREE.Vector3());
  // Project to screen
  const projected = center.clone().project(cam);
  const rect = rendererDom.getBoundingClientRect();
  const sx = (projected.x * 0.5 + 0.5) * rect.width + rect.left;
  const sy = (-projected.y * 0.5 + 0.5) * rect.height + rect.top;
  // Offset to the right and slightly above
  const vp = getViewportSize();
  const popRect = partPopupEl.getBoundingClientRect();
  const tw = popRect.width || 180;
  const th = popRect.height || 80;
  let left = sx + 20;
  let top = sy - th / 2;
  if (left + tw > vp.width - 8) left = sx - tw - 20;
  if (top + th > vp.height - 8) top = vp.height - th - 8;
  if (top < 8) top = 8;
  left = Math.max(8, left);
  partPopupEl.style.left = `${left}px`;
  partPopupEl.style.top = `${top}px`;
}

// --- Selection logic ---
function selectPart(target) {
  // Unhighlight previous selection
  deselectPart();

  if (!target) return;
  selectedPart = target;
  selectedPartKey = partKey(target);

  // Highlight meshes
  applyHighlight(target.meshes);

  // BMS cells also get the existing scale animation
  if (target.type === "bms-cell") {
    highlightCell(target.id);
  }

  // Show popup
  const content = buildPartPopupContent(target);
  showPartPopup(content);
}

function deselectPart() {
  if (selectedPart) {
    removeHighlight(selectedPart.meshes);
    // Reset BMS cell highlight if it was a cell
    if (selectedPart.type === "bms-cell") {
      highlightCell(null);
    }
    selectedPart = null;
    selectedPartKey = null;
  }
  hidePartPopup();
  // Also close BMS detail panel if open
  const dp = document.querySelector("[data-detail-panel]");
  if (dp && dp.classList.contains("is-visible")) {
    cancelScheduledDetailRefresh();
    detailPendingForceGraph = false;
    dp.classList.remove("is-visible");
  }
}

// Per-frame: update popup position + live data
function updatePartPopupFrame() {
  if (!partPopupVisible || !selectedPart) return;
  // Position the popup at the anchor mesh
  if (activePageId === "bms") {
    positionPopupAtMesh(selectedPart.anchorMesh, camera, renderer.domElement);
  } else if (activePageId === "eload") {
    positionPopupAtMesh(selectedPart.anchorMesh, eloadCamera, eloadRenderer?.domElement);
  }
}

function refreshPartPopupData() {
  if (!partPopupVisible || !selectedPart) return;
  const content = buildPartPopupContent(selectedPart);
  updatePartPopupContent(content);
}

// --- Click handler (both pages) ---
window.addEventListener('click', onModelClick, false);

function onModelClick(event) {
  if (event.target instanceof Element) {
    const t = event.target;
    if (t.closest(".hud") || t.closest("[data-detail-panel]") || t.closest(".hover-tooltip")) return;
  }

  const { width, height } = getViewportSize();

  if (activePageId === "bms" && loadedModel) {
    mouse.x = (event.clientX / width) * 2 - 1;
    mouse.y = -(event.clientY / height) * 2 + 1;
    raycaster.setFromCamera(mouse, camera);
    const intersects = raycaster.intersectObjects(loadedModel.children, true);
    const target = identifyBmsClickTarget(intersects);
    const key = partKey(target);

    if (!target) {
      deselectPart();
      return;
    }
    // Toggle off if clicking the same part
    if (key === selectedPartKey) {
      deselectPart();
      return;
    }
    selectPart(target);
    // BMS cells also show the detail panel sidebar
    if (target.type === "bms-cell") {
      showDetail(target.id);
    }

  } else if (activePageId === "eload" && loadedEloadModel && eloadCamera) {
    eloadMouse.x = (event.clientX / width) * 2 - 1;
    eloadMouse.y = -(event.clientY / height) * 2 + 1;
    eloadRaycaster.setFromCamera(eloadMouse, eloadCamera);
    const intersects = eloadRaycaster.intersectObjects(loadedEloadModel.children, true);
    const target = identifyEloadClickTarget(intersects);
    const key = partKey(target);

    if (!target) {
      deselectPart();
      return;
    }
    if (key === selectedPartKey) {
      deselectPart();
      return;
    }
    selectPart(target);
  }
}

// --- UI & Data Logic ---
const packVoltageEl = document.querySelector("[data-pack-voltage]");
const packCurrentEl = document.querySelector("[data-pack-current]");
const packTempEl = document.querySelector("[data-pack-temp]");
const fanSpeed1El = document.querySelector("[data-fan-speed-1]");
const fanSpeed2El = document.querySelector("[data-fan-speed-2]");
const sysStatEl = document.querySelector("[data-sys-stat]");
const loadPresentEl = document.querySelector("[data-load-present]");
const thermalTrendEl = document.querySelector("[data-thermal-trend]");
const cellGridEl = document.querySelector(".cell-grid");
const detailPanel = document.querySelector("[data-detail-panel]");
const detailTitle = document.querySelector("[data-cell-title]");
const detailVoltage = document.querySelector("[data-cell-voltage]");
const detailTemp = document.querySelector("[data-cell-temperature]");
const detailCurrent = document.querySelector("[data-cell-current]");
const detailDelta = document.querySelector("[data-cell-delta]");
const detailTrendLatest = document.querySelector("[data-cell-trend-latest]");
const detailTrendMin = document.querySelector("[data-cell-trend-min]");
const detailTrendMax = document.querySelector("[data-cell-trend-max]");
const detailTrendLine = document.querySelector("[data-cell-trend-line]");
const detailTrendArea = document.querySelector("[data-cell-trend-area]");
const closePanelBtn = document.querySelector("[data-close-panel]");
const dataPulseEl = document.getElementById("data-pulse");
const STATUS_WAITING = "Waiting for Data";
const STATUS_CONNECTED = "Connected";
const STATUS_SIMULATION = "Simulation Mode";
const STATUS_SIM_COMMAND_BLOCKED = "Simulation ON: hardware commands blocked";
const CELL_HISTORY_LENGTH = 45;
const TREND_WIDTH = 260;
const TREND_HEIGHT = 90;
const TREND_PADDING = 8;

// --- E-Load Shunt Trend History (reuses same circular buffer pattern as BMS cells) ---
const ELOAD_HISTORY_LENGTH = 45;
const eloadShuntHistory = { s1: [], s2: [], s3: [], s4: [] };

const DETAIL_REFRESH_INTERVAL_MS = 120;
const BASE_UI_WIDTH = 1400;
const BASE_UI_HEIGHT = 860;
const MIN_UI_SCALE = 0.65;
const MAX_UI_SCALE = 1.0;
const COMPACT_LAYOUT_ENTER_WIDTH = 980;
const COMPACT_LAYOUT_EXIT_WIDTH = 1080;
const CELL_VOLTAGE_MIN = 2.85;
const CELL_VOLTAGE_MAX = 4.2;
const CELL_VOLTAGE_LOW = 2.85;
const CELL_VOLTAGE_HIGH = 3.8;
const LOW_CELL_MIN_FILL_PERCENT = 10;
const CELL_COUNT = 10;
const FAN_ESTIMATED_RPM_PER_DUTY = 27;

let currentState = createBlankState();
const cellVoltageHistory = new Map();
const trendDirtyCells = new Set();
let compactLayoutEnabled = false;
let resizeRafId = 0;
let detailRefreshTimer = 0;
let detailPendingForceGraph = false;
let lastDetailRenderTs = 0;
let lastPulseDurationSec = null;
let backendConnectionState = false;
let hasRealTelemetry = false;
let simulationEnabled = false;
let simulationIntervalId = null;
let simulationStatusResetTimer = 0;
let lastRealDashboardPayload = null;
let simulationRestoreTimer = 0;

function createBlankState() {
  return {
    cells: Array.from({ length: CELL_COUNT }, (_, i) => ({
      id: i + 1,
      voltage: null,
      temperature: null,
    })),
    pack_current: null,
    fan1: { rpm: 0 },
    fan2: { rpm: 0 },
    fan_control: { auto: true, duty: 0 },
    bal_status: { enabled: false, threshold: 15, mask: 0 },
    eload: {
      enabled: false,
      i_set: 0,
      dac: 0,
      vout: 0,
      v: null,
      s1: 0,
      s2: 0,
      s3: 0,
      s4: 0,
      v_set: 0,
    },
    sys_stat: null,
    load_present: null,
  };
}

function populateCellGrid() {
  const fragment = document.createDocumentFragment();
  currentState.cells.forEach((cell) => {
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
      <span class="bal-indicator"></span>
    `;
    card.dataset.cellId = cell.id;
    card.addEventListener("click", () => {
      const isCurrentlySelected = highlightedCellId === cell.id && detailPanel.classList.contains("is-visible");

      if (isCurrentlySelected) {
        // Toggle off
        cancelScheduledDetailRefresh();
        detailPendingForceGraph = false;
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

function renderDetail(cellId, options = {}) {
  const {
    ensureVisible = false,
    reposition = false,
    forceGraph = false,
  } = options;

  const cell = currentState?.cells.find((c) => c.id === cellId);
  const packCurrent = isFiniteNumber(currentState?.pack_current) ? currentState.pack_current : null;
  if (!Number.isInteger(cellId)) return;
  detailTitle.textContent = `Cell ${cellId.toString().padStart(2, "0")}`;
  if (detailCurrent) {
    detailCurrent.textContent = packCurrent !== null ? `${packCurrent.toFixed(3)} A` : "-- A";
  }

  if (cell && isFiniteNumber(cell.voltage)) {
    detailVoltage.textContent = `${cell.voltage.toFixed(3)} V`;
    detailDelta.textContent = `${((cell.voltage - 3.8) * 1000).toFixed(0)} mV`;
    if (isFiniteNumber(cell.temperature) && cell.temperature > -200) {
      detailTemp.textContent = `${cell.temperature.toFixed(1)} \u00B0C`;
    } else {
      detailTemp.textContent = "N/A";
    }
  } else {
    detailVoltage.textContent = "-- V";
    detailTemp.textContent = "-- \u00B0C";
    detailDelta.textContent = "-- mV";
  }

  if (forceGraph || trendDirtyCells.has(cellId)) {
    drawCellTrend(cellId);
    trendDirtyCells.delete(cellId);
  }

  if (reposition) {
    positionDetailPanel();
  }
  if (ensureVisible) {
    detailPanel.classList.add("is-visible");
  }
  lastDetailRenderTs = performance.now();
}

function cancelScheduledDetailRefresh() {
  if (!detailRefreshTimer) return;
  window.clearTimeout(detailRefreshTimer);
  detailRefreshTimer = 0;
}

function scheduleDetailRefresh(forceGraph = false) {
  if (!detailPanel.classList.contains("is-visible") || !Number.isInteger(highlightedCellId)) {
    detailPendingForceGraph = false;
    cancelScheduledDetailRefresh();
    return;
  }

  detailPendingForceGraph = detailPendingForceGraph || forceGraph;
  if (detailRefreshTimer) return;

  const elapsedSinceLast = performance.now() - lastDetailRenderTs;
  const delayMs = Math.max(0, DETAIL_REFRESH_INTERVAL_MS - elapsedSinceLast);
  detailRefreshTimer = window.setTimeout(() => {
    detailRefreshTimer = 0;
    if (!detailPanel.classList.contains("is-visible") || !Number.isInteger(highlightedCellId)) {
      detailPendingForceGraph = false;
      return;
    }

    renderDetail(highlightedCellId, {
      ensureVisible: false,
      reposition: false,
      forceGraph: detailPendingForceGraph,
    });
    detailPendingForceGraph = false;
  }, delayMs);
}

function showDetail(cellId) {
  cancelScheduledDetailRefresh();
  detailPendingForceGraph = false;
  renderDetail(cellId, {
    ensureVisible: true,
    reposition: true,
    forceGraph: true,
  });
}

closePanelBtn.addEventListener("click", () => {
  cancelScheduledDetailRefresh();
  detailPendingForceGraph = false;
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

    if (gsap) {
      gsap.to(entry.mesh.scale, {
        x: isSelected ? 1.1 : 1,
        y: isSelected ? 1.1 : 1,
        z: isSelected ? 1.1 : 1,
        duration: 0.4,
        ease: "back.out(1.7)"
      });
    } else {
      entry.mesh.scale.set(isSelected ? 1.1 : 1, isSelected ? 1.1 : 1, isSelected ? 1.1 : 1);
    }

    if (isSelected) {
      const materials = Array.isArray(entry.mesh.material) ? entry.mesh.material : [entry.mesh.material];
      materials.forEach((material) => {
        if (!material?.emissive) return;
        material.emissive.setHex(0x0a84ff);
        material.emissiveIntensity = 0.5;
      });
    } else {
      // Let animateCells handle the rest
      const materials = Array.isArray(entry.mesh.material) ? entry.mesh.material : [entry.mesh.material];
      materials.forEach((material) => {
        if (!material?.emissive) return;
        material.emissive.setHex(0x000000);
        material.emissiveIntensity = 0;
      });
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
  const durationSec = Number(pulseDuration.toFixed(2));
  if (lastPulseDurationSec !== null && Math.abs(lastPulseDurationSec - durationSec) < 0.04) {
    return;
  }
  lastPulseDurationSec = durationSec;

  // Update pulse speed without forcing reflow/class toggles.
  dataPulseEl.style.setProperty("--pulse-duration", `${durationSec}s`);
  dataPulseEl.style.animationDuration = `${durationSec}s`;
}

function applySimulationStatusIndicator(label = STATUS_SIMULATION) {
  if (!thermalTrendEl || !dataPulseEl) return;
  thermalTrendEl.textContent = label;
  dataPulseEl.classList.remove("status__dot--connected");
  dataPulseEl.classList.remove("status__dot--waiting");
  dataPulseEl.classList.add("status__dot--simulation");
}

function setConnectionStatus(connected, source = "backend") {
  if (!thermalTrendEl || !dataPulseEl) return;

  const nextConnected = Boolean(connected);
  if (source === "backend") {
    backendConnectionState = nextConnected;
    if (!nextConnected) {
      hasRealTelemetry = false;
    }
    if (simulationEnabled) {
      return;
    }
  }

  isBackendConnected = nextConnected;
  // Sync the serial config panel with backend connection state
  if (typeof window.__bmsSyncSerialConfigPanel === "function") {
    window.__bmsSyncSerialConfigPanel(isBackendConnected);
  }
  startConnectionTransition(isBackendConnected);
  if (isBackendConnected) {
    cancelViewResetTransition();
  } else {
    startViewResetTransition();
  }
  thermalTrendEl.textContent = isBackendConnected ? STATUS_CONNECTED : STATUS_WAITING;
  dataPulseEl.classList.toggle("status__dot--connected", isBackendConnected);
  dataPulseEl.classList.toggle("status__dot--waiting", !isBackendConnected);
  dataPulseEl.classList.remove("status__dot--simulation");

  if (!isBackendConnected) {
    fanSpinRpm = 0;
    lastPulseDurationSec = null;
    dataPulseEl.style.removeProperty("--pulse-duration");
    dataPulseEl.style.animationDuration = "";
    fanMeshes.forEach((entry) => {
      if (entry) {
        entry.rpm = 0;
      }
    });
    // Reverse E-Load reveal when backend disconnects (unless simulation is active).
    // eloadHasRevealed check first — it short-circuits safely before eloadSimulationEnabled
    // which may not yet be initialized during early setConnectionStatus(false) calls.
    if (eloadHasRevealed && !eloadSimulationEnabled) {
      eloadHasRevealed = false;
      startEloadReveal(false);
    }
  }
}

function isFiniteNumber(value) {
  return typeof value === "number" && Number.isFinite(value);
}

function parseRpmValue(value) {
  if (typeof value === "number" && Number.isFinite(value)) {
    return Math.max(0, value);
  }
  if (typeof value === "string") {
    const match = value.match(/-?\d+(?:\.\d+)?/);
    if (match) {
      const parsed = Number.parseFloat(match[0]);
      if (Number.isFinite(parsed)) {
        return Math.max(0, parsed);
      }
    }
  }
  return null;
}

function clamp(value, min, max) {
  return Math.min(max, Math.max(min, value));
}

function fanDutyFromAutoTemperature(tempC) {
  const points = [
    { temp: 30, duty: 20 },
    { temp: 35, duty: 35 },
    { temp: 40, duty: 55 },
    { temp: 45, duty: 75 },
    { temp: 50, duty: 100 },
  ];

  if (!isFiniteNumber(tempC)) return points[0].duty;
  if (tempC <= points[0].temp) return points[0].duty;

  for (let i = 1; i < points.length; i += 1) {
    const lo = points[i - 1];
    const hi = points[i];
    if (tempC <= hi.temp) {
      const alpha = (tempC - lo.temp) / (hi.temp - lo.temp);
      return Math.round(lo.duty + alpha * (hi.duty - lo.duty));
    }
  }

  return points[points.length - 1].duty;
}

function estimateRpmFromDuty(duty) {
  const clampedDuty = clamp(Math.round(Number(duty) || 0), 0, 100);
  return clampedDuty * FAN_ESTIMATED_RPM_PER_DUTY;
}

function simulationFanDutyFromCells(cells) {
  if (!isFanAuto) {
    return clamp(parseInt(fanSlider.value, 10) || 0, 0, 100);
  }
  const hottest = (Array.isArray(cells) ? cells : [])
    .map((cell) => cell?.temperature)
    .filter((temp) => isFiniteNumber(temp) && temp > -200)
    .reduce((acc, temp) => Math.max(acc, temp), -Infinity);
  return Number.isFinite(hottest) ? fanDutyFromAutoTemperature(hottest) : 20;
}

function normalizeSetpoint(rawValue, min, max) {
  const parsed = parseFloat(rawValue);
  if (!Number.isFinite(parsed)) {
    return min;
  }
  return Math.round(clamp(parsed, min, max) * 100) / 100;
}

function syncSetpointControl(slider, input, value) {
  const normalized = normalizeSetpoint(value, parseFloat(slider.min), parseFloat(slider.max));
  const text = normalized.toFixed(2);
  slider.value = text;
  input.value = text;
  updateSliderUI(slider);
}

function updateCellVoltageHistory(cells) {
  if (!Array.isArray(cells)) return;

  cells.forEach((cell) => {
    if (!Number.isInteger(cell?.id) || !isFiniteNumber(cell?.voltage)) return;

    if (!cellVoltageHistory.has(cell.id)) {
      cellVoltageHistory.set(cell.id, []);
    }

    const history = cellVoltageHistory.get(cell.id);
    history.push(cell.voltage);
    if (history.length > CELL_HISTORY_LENGTH) {
      history.shift();
    }
    trendDirtyCells.add(cell.id);
  });
}

function drawCellTrend(cellId) {
  if (!detailTrendLine || !detailTrendArea || !detailTrendMin || !detailTrendMax || !detailTrendLatest) return;

  const history = cellVoltageHistory.get(cellId) || [];
  if (!history.length) {
    detailTrendLine.setAttribute("d", "");
    detailTrendArea.setAttribute("d", "");
    detailTrendMin.textContent = "-- V";
    detailTrendMax.textContent = "-- V";
    detailTrendLatest.textContent = "-- V";
    return;
  }

  const values = history.slice(-CELL_HISTORY_LENGTH);
  const latest = values[values.length - 1];

  let minV = Math.min(...values);
  let maxV = Math.max(...values);
  if (Math.abs(maxV - minV) < 0.003) {
    minV -= 0.002;
    maxV += 0.002;
  }

  const range = maxV - minV;
  const usableWidth = TREND_WIDTH - TREND_PADDING * 2;
  const usableHeight = TREND_HEIGHT - TREND_PADDING * 2;

  const points = values.map((value, idx) => {
    const t = values.length === 1 ? 0 : idx / (values.length - 1);
    const x = TREND_PADDING + t * usableWidth;
    const y = TREND_PADDING + (1 - (value - minV) / range) * usableHeight;
    return { x, y };
  });

  const linePath = points
    .map((pt, idx) => `${idx === 0 ? "M" : "L"}${pt.x.toFixed(2)} ${pt.y.toFixed(2)}`)
    .join(" ");

  const floorY = (TREND_HEIGHT - TREND_PADDING).toFixed(2);
  const areaPath = `${linePath} L${points[points.length - 1].x.toFixed(2)} ${floorY} L${points[0].x.toFixed(2)} ${floorY} Z`;

  detailTrendLine.setAttribute("d", linePath);
  detailTrendArea.setAttribute("d", areaPath);
  detailTrendMin.textContent = `${minV.toFixed(3)} V`;
  detailTrendMax.textContent = `${maxV.toFixed(3)} V`;
  detailTrendLatest.textContent = `${latest.toFixed(3)} V`;
}

// --- E-Load Shunt Trend Graph Drawing (same pattern as BMS drawCellTrend) ---
function drawEloadShuntTrend(channel) {
  const lineEl = document.getElementById(`eload-trend-line-${channel}`);
  const areaEl = document.getElementById(`eload-trend-area-${channel}`);
  const minEl = document.getElementById(`eload-trend-min-${channel}`);
  const maxEl = document.getElementById(`eload-trend-max-${channel}`);
  const latestEl = document.getElementById(`eload-trend-latest-${channel}`);
  if (!lineEl || !areaEl) return;

  const history = eloadShuntHistory[channel] || [];
  if (!history.length) {
    lineEl.setAttribute("d", "");
    areaEl.setAttribute("d", "");
    if (minEl) minEl.textContent = "-- mV";
    if (maxEl) maxEl.textContent = "-- mV";
    if (latestEl) latestEl.textContent = "-- mV";
    return;
  }

  const values = history.slice(-ELOAD_HISTORY_LENGTH);
  const latest = values[values.length - 1];

  let minV = Math.min(...values);
  let maxV = Math.max(...values);
  if (Math.abs(maxV - minV) < 0.5) {
    minV -= 0.5;
    maxV += 0.5;
  }

  const range = maxV - minV;
  const usableWidth = TREND_WIDTH - TREND_PADDING * 2;
  const usableHeight = TREND_HEIGHT - TREND_PADDING * 2;

  const points = values.map((value, idx) => {
    const t = values.length === 1 ? 0 : idx / (values.length - 1);
    const x = TREND_PADDING + t * usableWidth;
    const y = TREND_PADDING + (1 - (value - minV) / range) * usableHeight;
    return { x, y };
  });

  const linePath = points
    .map((pt, idx) => `${idx === 0 ? "M" : "L"}${pt.x.toFixed(2)} ${pt.y.toFixed(2)}`)
    .join(" ");

  const floorY = (TREND_HEIGHT - TREND_PADDING).toFixed(2);
  const areaPath = `${linePath} L${points[points.length - 1].x.toFixed(2)} ${floorY} L${points[0].x.toFixed(2)} ${floorY} Z`;

  lineEl.setAttribute("d", linePath);
  areaEl.setAttribute("d", areaPath);
  if (minEl) minEl.textContent = `${minV.toFixed(0)} mV`;
  if (maxEl) maxEl.textContent = `${maxV.toFixed(0)} mV`;
  if (latestEl) latestEl.textContent = `${latest.toFixed(0)} mV`;
}

function positionDetailPanel() {
  if (!detailPanel || !cellGridEl) return;
  const { width, height } = getViewportSize();

  // In compact layout we keep the bottom-sheet behavior defined in CSS.
  if (document.body.classList.contains("compact-layout")) {
    detailPanel.style.left = "";
    detailPanel.style.top = "";
    detailPanel.style.right = "";
    detailPanel.style.bottom = "";
    return;
  }

  const gridRect = cellGridEl.getBoundingClientRect();
  const panelWidth = detailPanel.offsetWidth || 340;
  const panelHeight = detailPanel.offsetHeight || 420;
  const gap = 16;
  const viewportPadding = 12;

  // Prefer placing the panel immediately to the right of the 10-cell panel.
  let left = gridRect.right + gap;
  const maxLeft = width - panelWidth - viewportPadding;
  if (left > maxLeft) {
    const leftOfGrid = gridRect.left - panelWidth - gap;
    left = leftOfGrid >= viewportPadding ? leftOfGrid : maxLeft;
  }

  let top = gridRect.top;
  const maxTop = height - panelHeight - viewportPadding;
  top = Math.min(Math.max(top, viewportPadding), Math.max(maxTop, viewportPadding));

  detailPanel.style.left = `${Math.round(left)}px`;
  detailPanel.style.top = `${Math.round(top)}px`;
  detailPanel.style.right = "auto";
  detailPanel.style.bottom = "auto";
}

window.setConnectionStatus = setConnectionStatus;
setConnectionStatus(false);

// Compatibility helper for explicit hard-snap requests.
// The startup handoff now prefers __bmsSetStartupConnectionTarget().
window.setConnectionStatusInstant = function (connected) {
  const target = connected ? 1 : 0;
  connectionVisualProgress = target;
  connectionTransitionTo = target;
  connectionTransitionFrom = target;
  connectionTransitionActive = false;
  applyShellTransparency(target);
  applyModelConnectionPose(target);
  setConnectionStatus(connected);
};

function updateHud(data) {
  const cells = Array.isArray(data.cells) ? data.cells : [];
  const validVoltageCells = cells.filter((cell) => isFiniteNumber(cell?.voltage));
  // Treat -273.0°C (absolute zero) as "no sensor connected" sentinel
  const validTemps = cells
    .map((cell) => cell?.temperature)
    .filter((temp) => isFiniteNumber(temp) && temp > -200);
  const packCurrent = isFiniteNumber(data.pack_current) ? data.pack_current : null;

  const packVoltage = validVoltageCells.reduce((acc, cell) => acc + cell.voltage, 0);
  packVoltageEl.textContent = validVoltageCells.length ? `${packVoltage.toFixed(1)} V` : "-- V";
  if (packCurrentEl) {
    packCurrentEl.textContent = packCurrent !== null ? `${packCurrent.toFixed(3)} A` : "-- A";
  }
  packTempEl.textContent = validTemps.length ? `${Math.max(...validTemps).toFixed(1)} \u00B0C` : "N/A";

  const fan1TelemetryRpm = parseRpmValue(data.fan1?.rpm)
    ?? parseRpmValue(data.fan1_rpm)
    ?? parseRpmValue(data.fan?.rpm)
    ?? parseRpmValue(currentState.fan1?.rpm)
    ?? null;
  const fan2TelemetryRpm = parseRpmValue(data.fan2?.rpm)
    ?? parseRpmValue(data.fan2_rpm)
    ?? parseRpmValue(data.fan?.rpm2)
    ?? parseRpmValue(currentState.fan2?.rpm)
    ?? null;
  const fan1Rpm = (fan1TelemetryRpm != null && fan1TelemetryRpm > 0)
    ? fan1TelemetryRpm
    : (fan2TelemetryRpm != null ? fan2TelemetryRpm : (fan1TelemetryRpm ?? 0));
  fanSpinRpm = Math.max(fan1Rpm, 0);
  fanMeshes.forEach((entry) => {
    if (!entry) return;
    entry.rpm = fanSpinRpm;
  });
  // Show actual RPM (including 0) when we have telemetry; "-- RPM" only when no data
  const hasFanData = fan1TelemetryRpm != null || fan2TelemetryRpm != null;
  fanSpeed1El.textContent = hasFanData ? `${Math.round(fan1Rpm).toLocaleString()} RPM` : "-- RPM";
  fanSpeed2El.textContent = "-- RPM";

  // Update SYS_STAT and Load Present
  if (sysStatEl) {
    sysStatEl.textContent = isFiniteNumber(data.sys_stat) ? `0x${data.sys_stat.toString(16).toUpperCase().padStart(2, "0")} (${data.sys_stat})` : "--";
  }
  if (loadPresentEl) {
    loadPresentEl.textContent = isFiniteNumber(data.load_present) ? (data.load_present ? "Yes" : "No") : "--";
  }

  // Trigger data pulse with speed based on data rate
  if (fan1Rpm > 0) {
    triggerDataPulse(fan1Rpm);
  }

  const cellsById = new Map();
  cells.forEach((cell) => {
    if (Number.isInteger(cell?.id)) {
      cellsById.set(cell.id, cell);
    }
  });

  document.querySelectorAll(".cell-card").forEach((card) => {
    const id = Number(card.dataset.cellId);
    const cell = cellsById.get(id);
    const valueEl = card.querySelector(".cell-card__value");
    if (!valueEl) return;

    const balIndicator = card.querySelector(".bal-indicator");
    if (balIndicator) {
      if (data.bal_status?.enabled && (data.bal_status.mask & (1 << (id - 1)))) {
        balIndicator.classList.add("active");
      } else {
        balIndicator.classList.remove("active");
      }
    }

    const levelEl = card.querySelector(".battery-level");
    if (!cell || !isFiniteNumber(cell.voltage)) {
      valueEl.textContent = "-- V";
      if (levelEl) {
        levelEl.style.height = "0%";
        levelEl.style.backgroundColor = "rgba(255, 255, 255, 0.25)";
      }
      return;
    }

    // Update text
    valueEl.textContent = `${cell.voltage.toFixed(3)} V`;

    // Update Battery Icon
    if (levelEl) {
      // Map configured minimum/maximum cell voltage to icon fill.
      const pct = Math.max(
        0,
        Math.min(100, ((cell.voltage - CELL_VOLTAGE_MIN) / (CELL_VOLTAGE_MAX - CELL_VOLTAGE_MIN)) * 100)
      );
      const displayPct = cell.voltage < CELL_VOLTAGE_LOW
        ? Math.max(pct, LOW_CELL_MIN_FILL_PERCENT)
        : pct;
      levelEl.style.height = `${displayPct}%`;

      // Color thresholds:
      // < 2.85V -> orange-red (low), 2.85V to 3.8V -> green (normal), > 3.8V -> red (overcharged).
      if (cell.voltage < CELL_VOLTAGE_LOW) levelEl.style.backgroundColor = 'orange';
      else if (cell.voltage <= CELL_VOLTAGE_HIGH) levelEl.style.backgroundColor = 'var(--success-color)';
      else levelEl.style.backgroundColor = 'var(--danger-color)';
    }
  });

  if (detailPanel.classList.contains("is-visible") && Number.isInteger(highlightedCellId)) {
    scheduleDetailRefresh(false);
  }
  refreshPartPopupData();
}

function colorForVoltage(voltage) {
  if (!isFiniteNumber(voltage)) {
    return new THREE.Color(0.55, 0.55, 0.55);
  }
  if (voltage < CELL_VOLTAGE_LOW) {
    return new THREE.Color(0xff9f0a); // orange (low)
  }
  if (voltage <= CELL_VOLTAGE_HIGH) {
    return new THREE.Color(0x30d158); // green (normal)
  }
  return new THREE.Color(0xff453a); // red (overcharged)
}

function updateCellColorTargets(data) {
  if (!Array.isArray(data.cells)) return;

  data.cells.forEach((cellData) => {
    if (!isFiniteNumber(cellData?.voltage)) return;

    const meshEntry = cellMeshes.find((entry) => entry.id === cellData.id);
    if (!meshEntry || highlightedCellId === meshEntry.id) return;
    if (!selectedCellUuidSet.has(meshEntry.mesh?.uuid)) return;

    const voltageColor = colorForVoltage(cellData.voltage);
    if (!meshEntry.targetColor) {
      meshEntry.targetColor = meshEntry.baseColor.clone();
    }
    meshEntry.targetColor.copy(voltageColor);
  });
}

function animateCells(deltaSeconds) {
  updateConnectionVisualState(performance.now());

  if (fanMeshes.length > 0) {
    fanMeshes.forEach((entry) => {
      if (!entry?.spinNode) return;
      const rpmForSpin = Math.max(0, Number(entry.rpm) || 0);
      if (rpmForSpin <= 0) return;

      const spinFactor = THREE.MathUtils.clamp(rpmForSpin / 4500, 0, 1);
      const spinSpeed = THREE.MathUtils.lerp(
        FAN_SPIN_BASE_RAD_PER_SEC,
        FAN_SPIN_MAX_RAD_PER_SEC,
        spinFactor,
      );
      const rotationDelta = deltaSeconds * spinSpeed;
      const spinNode = entry.spinNode || entry.mesh;
      const axisVector = entry.axisVector || axisVectorForLabel(entry.axis);
      if (!spinNode || !axisVector) return;
      const direction = entry.fanId === 2 ? -1 : 1;
      spinNode.rotateOnAxis(axisVector, direction * rotationDelta);
    });
  }

  cellMeshes.forEach((meshEntry) => {
    if (!meshEntry.targetColor || highlightedCellId === meshEntry.id) return;

    const materials = Array.isArray(meshEntry.mesh.material)
      ? meshEntry.mesh.material
      : [meshEntry.mesh.material];
    materials.forEach((material) => {
      if (!material?.color) return;
      material.color.lerp(meshEntry.targetColor, 0.18);
    });
  });
}

// --- Animation Loop ---
const clock = new THREE.Clock();

function tick() {
  const delta = clock.getDelta();

  if (activePageId === "bms") {
    updateViewResetTransition(performance.now());
    controls.update();

    // Gentle rotation of the whole model (disabled by default)
    if (AUTO_ROTATE_MODEL && loadedModel) {
      loadedModel.rotation.y += delta * 0.1;
    }

    animateCells(delta);
    renderer.render(scene, camera);
    updatePartPopupFrame();
    refreshPartPopupData();
  } else if (activePageId === "eload" && eloadRenderer && eloadScene && eloadCamera) {
    updateEloadRevealTransition(performance.now());
    if (eloadControls) eloadControls.update();
    updateEloadFanSpin(delta);
    updateEloadHeatVisualization(delta);
    eloadRenderer.render(eloadScene, eloadCamera);
    updatePartPopupFrame();
    refreshPartPopupData();
  }

  requestAnimationFrame(tick);
}

tick();

function getViewportSize() {
  const width = Math.max(
    320,
    Math.floor(window.innerWidth || document.documentElement.clientWidth || 0),
  );
  const height = Math.max(
    240,
    Math.floor(window.innerHeight || document.documentElement.clientHeight || 0),
  );

  return { width, height };
}

function updateResponsiveUiScale() {
  const { width, height } = getViewportSize();
  document.documentElement.style.setProperty("--viewport-width", `${width}px`);
  document.documentElement.style.setProperty("--viewport-height", `${height}px`);

  const isCompactLayout = compactLayoutEnabled
    ? width <= COMPACT_LAYOUT_EXIT_WIDTH
    : width <= COMPACT_LAYOUT_ENTER_WIDTH;
  compactLayoutEnabled = isCompactLayout;
  document.body.classList.toggle("compact-layout", isCompactLayout);

  if (isCompactLayout) {
    document.documentElement.style.setProperty("--ui-scale", "1");
    return;
  }

  const scaleByWidth = width / BASE_UI_WIDTH;
  const scaleByHeight = height / BASE_UI_HEIGHT;
  const scale = Math.max(
    MIN_UI_SCALE,
    Math.min(MAX_UI_SCALE, scaleByWidth, scaleByHeight),
  );
  document.documentElement.style.setProperty("--ui-scale", scale.toFixed(3));
}
// --- QWebChannel / Bridge Logic ---
let backendLink = null;

// Global command queue — Python polls this via runJavaScript
window.__bmsCommandQueue = [];
window.__bmsDrainCommands = function () {
  const cmds = window.__bmsCommandQueue.splice(0);
  return cmds.length ? JSON.stringify(cmds) : "";
};

// Initialize Channel
if (typeof QWebChannel !== "undefined" && window.qt?.webChannelTransport) {
  new QWebChannel(qt.webChannelTransport, function (channel) {
    backendLink = channel.objects.backend;
    if (backendLink) {
      console.log("Connected to backend bridge via QWebChannel");
    } else {
      console.warn("Backend object not found in channel.objects");
    }
  });
} else {
  console.warn("QWebChannel not found. Running in standalone/mock mode?");
}

function sendBackendCommand(cmd) {
  if (simulationEnabled) {
    console.warn(`[BMS] Simulation ON: blocked hardware command -> ${cmd}`);
    if (simulationStatusResetTimer) {
      window.clearTimeout(simulationStatusResetTimer);
    }
    applySimulationStatusIndicator(STATUS_SIM_COMMAND_BLOCKED);
    simulationStatusResetTimer = window.setTimeout(() => {
      simulationStatusResetTimer = 0;
      if (simulationEnabled) {
        applySimulationStatusIndicator(STATUS_SIMULATION);
      }
    }, 1600);
    return;
  }

  // Push to command queue (Python polls this)
  window.__bmsCommandQueue.push(cmd);
  console.log("[BMS] Queued command:", cmd);

  // Also try QWebChannel direct call
  if (backendLink && backendLink.sendCommand) {
    try {
      backendLink.sendCommand(cmd);
    } catch (err) {
      console.error("sendCommand call failed:", err);
    }
  }
}

// --- E-Load UI Logic ---
// Per-channel toggle controls (replaces single eload-toggle)
const eloadChToggles = [
  document.getElementById("eload-ch1-toggle"),
  document.getElementById("eload-ch2-toggle"),
  document.getElementById("eload-ch3-toggle"),
  document.getElementById("eload-ch4-toggle"),
];
const eloadToggle = { get checked() { return eloadChToggles.some(t => t?.checked); } };  // compat shim

// Telemetry Elements
const telemVoltage = document.getElementById("telem-voltage");
const telemIset = document.getElementById("telem-iset");
const telemDac = document.getElementById("telem-dac");
const telemVout = document.getElementById("telem-vout");
const telemEn = document.getElementById("telem-en");
const telemS1 = document.getElementById("telem-s1");
const telemS2 = document.getElementById("telem-s2");
const telemS3 = document.getElementById("telem-s3");
const telemS4 = document.getElementById("telem-s4");

// Charge Mode Elements
const chargeOffBtn = document.getElementById("charge-off-btn");
const chargeOnBtn = document.getElementById("charge-on-btn");
let isChargeMode = false;

// Cell Balancing Elements
const balOffBtn = document.getElementById("bal-off-btn");
const balAltBtn = document.getElementById("bal-alt-btn");
const balControls = document.getElementById("bal-controls");
const balSlider = document.getElementById("bal-slider");
const balValue = document.getElementById("bal-value");

let isBalEnabled = false;

if (balOffBtn) {
  balOffBtn.addEventListener("click", () => {
    if (!isBalEnabled) return;
    isBalEnabled = false;
    balOffBtn.classList.add("active");
    if (balAltBtn) balAltBtn.classList.remove("active");
    if (balControls) balControls.classList.add("disabled");
    sendBackendCommand("BMS:BAL:ALT:OFF");
  });
}

if (balAltBtn) {
  balAltBtn.addEventListener("click", () => {
    if (isBalEnabled) return;
    isBalEnabled = true;
    balAltBtn.classList.add("active");
    if (balOffBtn) balOffBtn.classList.remove("active");
    if (balControls) balControls.classList.remove("disabled");
    sendBackendCommand("BMS:BAL:ALT:ON");
  });
}

if (balSlider) {
  balSlider.addEventListener("change", (e) => {
    const val = parseInt(e.target.value, 10);
    sendBackendCommand(`BMS:BAL:THRESH:${val}`);
  });
  balSlider.addEventListener("input", (e) => {
    if (balValue) balValue.textContent = `${e.target.value} mV`;
    updateSliderUI(e.target);
  });
}

// Charge Mode Handlers (after balance elements are declared)
if (chargeOffBtn) {
  chargeOffBtn.addEventListener("click", () => {
    if (!isChargeMode) return;
    isChargeMode = false;
    chargeOffBtn.classList.add("active");
    if (chargeOnBtn) chargeOnBtn.classList.remove("active");
    sendBackendCommand("BMS:CHARGE:OFF");
  });
}

if (chargeOnBtn) {
  chargeOnBtn.addEventListener("click", () => {
    if (isChargeMode) return;
    isChargeMode = true;
    chargeOnBtn.classList.add("active");
    if (chargeOffBtn) chargeOffBtn.classList.remove("active");
    // Deactivate manual balancing when entering charge mode
    isBalEnabled = false;
    if (balAltBtn) balAltBtn.classList.remove("active");
    if (balOffBtn) balOffBtn.classList.add("active");
    if (balControls) balControls.classList.add("disabled");
    sendBackendCommand("BMS:CHARGE:ON");
  });
}

// Fan Elements
const fanAutoBtn = document.getElementById("fan-auto-btn");
const fanManualBtn = document.getElementById("fan-manual-btn");
const fanManualControls = document.getElementById("fan-manual-controls");
const fanSlider = document.getElementById("fan-slider");
const fanValue = document.getElementById("fan-value");
const simulateDataToggle = document.getElementById("simulate-data-toggle");
const simulateDataModeEl = document.getElementById("simulate-data-mode");

let isFanAuto = true;

// -- E-Load Per-Channel Toggle Controls --
eloadChToggles.forEach((toggle, idx) => {
  if (!toggle) return;
  toggle.addEventListener("change", (e) => {
    const ch = idx + 1;
    const state = e.target.checked ? 1 : 0;
    sendBackendCommand(`ELOAD:CH:${ch}:${state}`);
  });
});

const eloadAllOnBtn = document.getElementById("eload-all-on");
const eloadAllOffBtn = document.getElementById("eload-all-off");
if (eloadAllOnBtn) {
  eloadAllOnBtn.addEventListener("click", () => {
    sendBackendCommand("ELOAD:ON");
    eloadChToggles.forEach((t) => { if (t) t.checked = true; });
  });
}
if (eloadAllOffBtn) {
  eloadAllOffBtn.addEventListener("click", () => {
    sendBackendCommand("ELOAD:OFF");
    eloadChToggles.forEach((t) => { if (t) t.checked = false; });
  });
}

// -- E-Load Fan Visualization Controls --
const eloadFanToggle = document.getElementById("eload-fan-toggle");
const eloadFanSpeedSlider = document.getElementById("eload-fan-speed-slider");
const eloadFanSpeedValue = document.getElementById("eload-fan-speed-value");
const eloadFanSpeedControls = document.getElementById("eload-fan-speed-controls");

if (eloadFanToggle) {
  eloadFanToggle.addEventListener("change", (e) => {
    eloadFanSpinEnabled = e.target.checked;
    if (eloadFanSpeedControls) {
      eloadFanSpeedControls.classList.toggle("disabled", !eloadFanSpinEnabled);
    }
  });
}

if (eloadFanSpeedSlider) {
  eloadFanSpeedSlider.addEventListener("input", (e) => {
    const val = parseFloat(e.target.value);
    eloadFanSpinSpeed = val / 100;
    if (eloadFanSpeedValue) eloadFanSpeedValue.textContent = `${Math.round(val)}%`;
  });
}

// -- E-Load Heat Visualization Controls --
const eloadHeatToggle = document.getElementById("eload-heat-toggle");
const eloadHeatIntensitySlider = document.getElementById("eload-heat-intensity-slider");
const eloadHeatIntensityValue = document.getElementById("eload-heat-intensity-value");
const eloadHeatIntensityControls = document.getElementById("eload-heat-intensity-controls");

if (eloadHeatToggle) {
  eloadHeatToggle.addEventListener("change", (e) => {
    eloadHeatVizEnabled = e.target.checked;
    if (eloadHeatIntensityControls) {
      eloadHeatIntensityControls.classList.toggle("disabled", !eloadHeatVizEnabled);
    }
    // Swap materials: thermal shader <-> original reflective
    eloadThermalEntries.forEach(({ mesh, originalMat }) => {
      mesh.material = eloadHeatVizEnabled ? eloadThermalShaderMat : originalMat;
    });
  });
}

if (eloadHeatIntensitySlider) {
  eloadHeatIntensitySlider.addEventListener("input", (e) => {
    const val = parseFloat(e.target.value);
    eloadHeatIntensity = val / 100;
    if (eloadHeatIntensityValue) eloadHeatIntensityValue.textContent = `${Math.round(val)}%`;
  });
}

// --- Glass Slider UI Sync ---
function updateSliderUI(input) {
  const slider = input.closest(".glass-slider");
  if (!slider) return;

  const track = slider.querySelector(".glass-slider__track");
  const progress = slider.querySelector(".glass-slider__progress");
  const thumb = slider.querySelector(".glass-slider__thumb");
  if (!track || !progress || !thumb) return;

  const min = input.min ? parseFloat(input.min) : 0;
  const max = input.max ? parseFloat(input.max) : 100;
  const val = parseFloat(input.value);
  const percent = ((val - min) / (max - min)) * 100;

  progress.style.width = `${percent}%`;
  const trackWidth = track.clientWidth;
  if (!trackWidth) return;
  const px = track.offsetLeft + (percent / 100) * trackWidth;
  thumb.style.left = `${px}px`;
}

const sliderInputs = document.querySelectorAll(".glass-slider input[type=range]");

sliderInputs.forEach((input) => {
  updateSliderUI(input);
  input.addEventListener("input", () => updateSliderUI(input));
  input.addEventListener("change", () => updateSliderUI(input));

  const thumb = input.closest(".glass-slider")?.querySelector(".glass-slider__thumb");
  if (thumb) {
    input.addEventListener("pointerdown", () => thumb.classList.add("active"));
    input.addEventListener("pointercancel", () => thumb.classList.remove("active"));
    input.addEventListener("blur", () => thumb.classList.remove("active"));
  }
});

window.addEventListener("pointerup", () => {
  document.querySelectorAll(".glass-slider__thumb.active").forEach((thumb) => {
    thumb.classList.remove("active");
  });
});


// -- Fan Control --
let _fanThrottleTimer = 0;
let _fanLastSendTime = 0;
let _fanPendingDuty = -1;

// Clear Faults button — writes 0x0F to SYS_STAT on BQ76930
const clearFaultsBtn = document.getElementById("clear-faults-btn");
if (clearFaultsBtn) {
  clearFaultsBtn.addEventListener("click", () => {
    sendBackendCommand("BMS:CLEAR_FAULTS");
  });
}

fanAutoBtn.addEventListener("click", () => {
  setFanMode(true);
  sendBackendCommand("FAN:AUTO");
  if (simulationEnabled) {
    mockStream();
  }
});

fanManualBtn.addEventListener("click", () => {
  setFanMode(false);
  sendBackendCommand("FAN:MANUAL");
  if (simulationEnabled) {
    mockStream();
  }
});

function _fanSendThrottled(duty) {
  const now = Date.now();
  const elapsed = now - _fanLastSendTime;
  if (elapsed >= 100) {
    // Send immediately
    _fanLastSendTime = now;
    _fanPendingDuty = -1;
    sendBackendCommand(`FAN:SET:${duty}`);
  } else {
    // Schedule trailing send for the latest value
    _fanPendingDuty = duty;
    if (!_fanThrottleTimer) {
      _fanThrottleTimer = window.setTimeout(() => {
        _fanThrottleTimer = 0;
        if (_fanPendingDuty >= 0) {
          _fanLastSendTime = Date.now();
          sendBackendCommand(`FAN:SET:${_fanPendingDuty}`);
          _fanPendingDuty = -1;
        }
      }, 100 - elapsed);
    }
  }
}

fanSlider.addEventListener("input", (e) => {
  const duty = clamp(parseInt(e.target.value, 10) || 0, 0, 100);
  fanSlider.value = duty.toString();
  fanValue.textContent = `${duty}%`;
  updateSliderUI(fanSlider);
  if (!isFanAuto) {
    _fanSendThrottled(duty);
  }
  if (simulationEnabled) {
    mockStream();
  }
});

fanSlider.addEventListener("change", (e) => {
  const duty = clamp(parseInt(e.target.value, 10) || 0, 0, 100);
  fanSlider.value = duty.toString();
  fanValue.textContent = `${duty}%`;
  updateSliderUI(fanSlider);
  // On release, cancel pending throttle and send final value immediately
  if (_fanThrottleTimer) { window.clearTimeout(_fanThrottleTimer); _fanThrottleTimer = 0; }
  _fanPendingDuty = -1;
  if (!isFanAuto) {
    _fanLastSendTime = Date.now();
    sendBackendCommand(`FAN:SET:${duty}`);
  }
  if (simulationEnabled) {
    mockStream();
  }
});

if (simulateDataToggle) {
  simulateDataToggle.addEventListener("change", (event) => {
    setSimulationMode(Boolean(event.target.checked));
  });
}

function setFanMode(auto) {
  isFanAuto = auto;
  if (auto) {
    fanAutoBtn.classList.add("active");
    fanManualBtn.classList.remove("active");
    fanManualControls.classList.add("disabled");
  } else {
    fanAutoBtn.classList.remove("active");
    fanManualBtn.classList.add("active");
    fanManualControls.classList.remove("disabled");
  }
}

// --- Real Data Injection ---
let pendingDashboardData = null;
let pendingDashboardFrame = 0;

function normalizeCells(cells) {
  const normalized = Array.from({ length: CELL_COUNT }, (_, i) => ({
    id: i + 1,
    voltage: null,
    temperature: null,
  }));

  if (!Array.isArray(cells)) {
    return normalized;
  }

  cells.forEach((cell, index) => {
    const id = Number.isInteger(cell?.id) ? cell.id : index + 1;
    if (id < 1 || id > CELL_COUNT) return;
    normalized[id - 1] = {
      id,
      voltage: isFiniteNumber(cell?.voltage) ? Number(cell.voltage) : null,
      temperature: isFiniteNumber(cell?.temperature) ? Number(cell.temperature) : null,
    };
  });
  return normalized;
}

function updateEloadTelemetry(eload) {
  // I_SET display (in mV)
  if (telemIset && isFiniteNumber(eload?.i_set)) {
    telemIset.textContent = `${eload.i_set.toFixed(1)} mV`;
  } else if (telemIset) {
    telemIset.textContent = "-- mV";
  }

  // DAC code display
  if (telemDac && isFiniteNumber(eload?.dac)) {
    telemDac.textContent = `${Math.round(eload.dac)}`;
  } else if (telemDac) {
    telemDac.textContent = "--";
  }

  // VOUT (DAC output voltage) display
  if (telemVout && isFiniteNumber(eload?.vout)) {
    telemVout.textContent = `${(eload.vout * 1000).toFixed(0)} mV`;
  } else if (telemVout) {
    telemVout.textContent = "-- mV";
  }

  // VSENSE display (in mV)
  if (telemVoltage && isFiniteNumber(eload?.v)) {
    telemVoltage.textContent = `${(eload.v * 1000).toFixed(0)} mV`;
  } else if (telemVoltage) {
    telemVoltage.textContent = "-- mV";
  }

  // S1-S4 sense channels in mV
  if (telemS1 && isFiniteNumber(eload?.s1)) {
    telemS1.textContent = `${(eload.s1 * 1000).toFixed(0)} mV`;
  } else if (telemS1) { telemS1.textContent = "-- mV"; }
  if (telemS2 && isFiniteNumber(eload?.s2)) {
    telemS2.textContent = `${(eload.s2 * 1000).toFixed(0)} mV`;
  } else if (telemS2) { telemS2.textContent = "-- mV"; }
  if (telemS3 && isFiniteNumber(eload?.s3)) {
    telemS3.textContent = `${(eload.s3 * 1000).toFixed(0)} mV`;
  } else if (telemS3) { telemS3.textContent = "-- mV"; }
  if (telemS4 && isFiniteNumber(eload?.s4)) {
    telemS4.textContent = `${(eload.s4 * 1000).toFixed(0)} mV`;
  } else if (telemS4) { telemS4.textContent = "-- mV"; }

  // EN status display (derived from per-channel states)
  if (telemEn) {
    const anyOn = eload?.ch1 || eload?.ch2 || eload?.ch3 || eload?.ch4 || eload?.enabled;
    if (anyOn !== undefined) {
      telemEn.textContent = anyOn ? "ACTIVE" : "ALL OFF";
      telemEn.style.color = anyOn ? "var(--success-color, #30d158)" : "var(--danger-color, #ff453a)";
    } else {
      telemEn.textContent = "--";
      telemEn.style.color = "";
    }
  }

  // Sync per-channel toggles with telemetry
  ["ch1", "ch2", "ch3", "ch4"].forEach((ch, idx) => {
    if (typeof eload?.[ch] === "boolean" && eloadChToggles[idx]) {
      eloadChToggles[idx].checked = eload[ch];
    }
  });

  // Push to shunt history circular buffers and redraw trend graphs
  ["s1", "s2", "s3", "s4"].forEach((ch) => {
    if (isFiniteNumber(eload?.[ch])) {
      const mv = eload[ch] * 1000;  // V -> mV for graph display
      eloadShuntHistory[ch].push(mv);
      if (eloadShuntHistory[ch].length > ELOAD_HISTORY_LENGTH) {
        eloadShuntHistory[ch].shift();
      }
      drawEloadShuntTrend(ch);
    }
  });


  // Power summary cards
  const eloadActualVoltageEl = document.getElementById("eload-actual-voltage");
  const eloadActualCurrentEl = document.getElementById("eload-actual-current");
  const eloadPowerEl = document.getElementById("eload-power");

  if (eloadActualVoltageEl) {
    eloadActualVoltageEl.textContent = isFiniteNumber(eload?.v)
      ? `${(eload.v * 1000).toFixed(0)} mV` : "-- mV";
  }
  if (eloadActualCurrentEl) {
    eloadActualCurrentEl.textContent = isFiniteNumber(eload?.i_set)
      ? `${eload.i_set.toFixed(1)} mV` : "-- mV";
  }
  if (eloadPowerEl) {
    eloadPowerEl.textContent = isFiniteNumber(eload?.vout)
      ? `${(eload.vout * 1000).toFixed(0)} mV` : "-- mV";
  }

  // Drive thermal visualization from I_SET as proxy for heat
  // (actual current not measurable with 1mOhm shunts)
  // I_SET range: 0-185.6 mV → normalize to 0-1
  if (isFiniteNumber(eload?.i_set) && eload.i_set > 0) {
    const heat = Math.min(eload.i_set / 185.6, 1.0);
    eloadFetHeatLevels = [heat, heat, heat, heat];
  } else {
    eloadFetHeatLevels = [0, 0, 0, 0];
  }
  refreshPartPopupData();
}

function clearMeshTargets() {
  cellMeshes.forEach((entry) => {
    if (!entry.targetColor) {
      entry.targetColor = entry.baseColor.clone();
    }
    entry.targetColor.copy(entry.baseColor);
    const materials = Array.isArray(entry.mesh.material) ? entry.mesh.material : [entry.mesh.material];
    materials.forEach((material) => {
      if (material?.emissive) {
        material.emissive.setHex(0x000000);
        material.emissiveIntensity = 0;
      }
    });
  });
}

function clearDashboardData(reason = "manual") {
  cancelScheduledDetailRefresh();
  detailPendingForceGraph = false;
  pendingDashboardData = null;
  currentState = createBlankState();
  cellVoltageHistory.clear();
  trendDirtyCells.clear();
  // Clear E-Load shunt trend history
  ["s1", "s2", "s3", "s4"].forEach((ch) => {
    eloadShuntHistory[ch] = [];
    drawEloadShuntTrend(ch);
  });
  clearMeshTargets();

  updateHud(currentState);
  updateEloadTelemetry(currentState.eload);
  fanSlider.value = "0";
  fanValue.textContent = "0%";
  updateSliderUI(fanSlider);

  if (Number.isInteger(highlightedCellId)) {
    renderDetail(highlightedCellId, {
      ensureVisible: false,
      reposition: false,
      forceGraph: true,
    });
  }

  // Disconnect transition is driven by setConnectionStatus(false) from backend
  // connection_status to preserve the same easing/style in reverse.
  if (reason === "startup") {
    setConnectionStatus(false);
  }
}

window.clearDashboardData = clearDashboardData;

function flushDashboardData() {
  pendingDashboardFrame = 0;
  const data = pendingDashboardData;
  pendingDashboardData = null;
  if (!data) return;

  if (Array.isArray(data.cells)) {
    currentState.cells = normalizeCells(data.cells);
    updateCellVoltageHistory(currentState.cells);
    updateCellColorTargets(currentState);
  }

  if (isFiniteNumber(data.pack_current)) {
    currentState.pack_current = data.pack_current;
  }
  if (data.fan1) {
    currentState.fan1 = data.fan1;
  }
  if (data.fan2) {
    currentState.fan2 = data.fan2;
  }
  if (data.fan_control) {
    currentState.fan_control = {
      auto: Boolean(data.fan_control.auto),
      duty: isFiniteNumber(data.fan_control.duty) ? Number(data.fan_control.duty) : 0,
    };
    setFanMode(currentState.fan_control.auto);
    const duty = clamp(currentState.fan_control.duty, 0, 100);
    fanSlider.value = `${Math.round(duty)}`;
    fanValue.textContent = `${Math.round(duty)}%`;
    updateSliderUI(fanSlider);
  }
  if (isFiniteNumber(data.sys_stat)) {
    currentState.sys_stat = data.sys_stat;
  }
  if (isFiniteNumber(data.load_present)) {
    currentState.load_present = data.load_present;
  }
  if (data.eload) {
    const e = data.eload;
    currentState.eload = {
      enabled: Boolean(e.enabled),
      i_set: isFiniteNumber(e.i_set) ? Number(e.i_set) : currentState.eload?.i_set || 0,
      dac: isFiniteNumber(e.dac) ? Number(e.dac) : currentState.eload?.dac || 0,
      vout: isFiniteNumber(e.vout) ? Number(e.vout) : currentState.eload?.vout || 0,
      v: isFiniteNumber(e.v) ? Number(e.v) : currentState.eload?.v || null,
      s1: isFiniteNumber(e.s1) ? Number(e.s1) : currentState.eload?.s1 || 0,
      s2: isFiniteNumber(e.s2) ? Number(e.s2) : currentState.eload?.s2 || 0,
      s3: isFiniteNumber(e.s3) ? Number(e.s3) : currentState.eload?.s3 || 0,
      s4: isFiniteNumber(e.s4) ? Number(e.s4) : currentState.eload?.s4 || 0,
      v_set: isFiniteNumber(e.v_set) ? Number(e.v_set) : currentState.eload?.v_set || 0,
      ch1: typeof e.ch1 === "boolean" ? e.ch1 : currentState.eload?.ch1 ?? true,
      ch2: typeof e.ch2 === "boolean" ? e.ch2 : currentState.eload?.ch2 ?? true,
      ch3: typeof e.ch3 === "boolean" ? e.ch3 : currentState.eload?.ch3 ?? true,
      ch4: typeof e.ch4 === "boolean" ? e.ch4 : currentState.eload?.ch4 ?? true,
    };

    // Trigger E-Load reveal on first real telemetry (or when connected)
    if (!eloadHasRevealed && !eloadSimulationEnabled) {
      eloadHasRevealed = true;
      startEloadReveal(true);
    }
  }

  if (data.bal_status) {
    currentState.bal_status = data.bal_status;
    const isEn = Boolean(data.bal_status.enabled);
    if (isEn) {
      isBalEnabled = true;
      if (balAltBtn) balAltBtn.classList.add("active");
      if (balOffBtn) balOffBtn.classList.remove("active");
      if (balControls) balControls.classList.remove("disabled");
    } else {
      isBalEnabled = false;
      if (balOffBtn) balOffBtn.classList.add("active");
      if (balAltBtn) balAltBtn.classList.remove("active");
      if (balControls) balControls.classList.add("disabled");
    }
  }

  if (data.bal_status && isFiniteNumber(data.bal_status.charge)) {
    const chg = Boolean(data.bal_status.charge);
    isChargeMode = chg;
    if (chg) {
      if (chargeOnBtn) chargeOnBtn.classList.add("active");
      if (chargeOffBtn) chargeOffBtn.classList.remove("active");
    } else {
      if (chargeOffBtn) chargeOffBtn.classList.add("active");
      if (chargeOnBtn) chargeOnBtn.classList.remove("active");
    }
  }

  updateHud(currentState);
  updateEloadTelemetry(currentState.eload);
}

function queueDashboardData(data) {
  pendingDashboardData = data;
  if (pendingDashboardFrame) return;
  pendingDashboardFrame = window.requestAnimationFrame(flushDashboardData);
}

// This function is called by Python: window.updateDashboard(jsonData)
window.updateDashboard = function (data) {
  const payload = data && typeof data === "object" ? data : {};
  const isSimulatedPayload = Boolean(payload.__simulated);

  if (!isSimulatedPayload) {
    hasRealTelemetry = true;
    backendConnectionState = true;
    lastRealDashboardPayload = payload;
  }

  if (simulationEnabled && !isSimulatedPayload) return;
  if (!simulationEnabled && isSimulatedPayload) return;

  queueDashboardData(payload);
};

function mockStream() {
  const cells = Array.from({ length: CELL_COUNT }, (_, index) => ({
    id: index + 1,
    voltage: Number((3.55 + Math.random() * 0.3).toFixed(3)),
    temperature: Number((24 + Math.random() * 6).toFixed(1)),
  }));
  const mockCurrent = Number(((Math.random() - 0.5) * 0.2).toFixed(3));
  const mockVoltage = Number((36 + Math.random() * 2).toFixed(2));
  const mockActualCurrent = Math.max(0, Math.abs(mockCurrent));
  const simulatedDuty = simulationFanDutyFromCells(cells);
  const simulatedRpm = estimateRpmFromDuty(simulatedDuty);

  window.updateDashboard({
    __simulated: true,
    cells,
    pack_current: mockCurrent,
    fan1: { rpm: simulatedRpm },
    fan2: { rpm: 0 },
    fan_control: {
      auto: isFanAuto,
      duty: simulatedDuty,
    },
    eload: {
      enabled: eloadToggle.checked,
      dac: 0,
      i_set: 0,
      vout: 0,
      v: 5.0 + (Math.random() - 0.5) * 0.1,
      s1: Math.random() * 0.002,
      s2: Math.random() * 0.002,
      s3: Math.random() * 0.002,
      s4: Math.random() * 0.002,
      v_set: 0,
    },
  });
}

function updateSimulationToggleUi() {
  if (simulateDataToggle) {
    simulateDataToggle.checked = simulationEnabled;
  }
  if (simulateDataModeEl) {
    simulateDataModeEl.textContent = simulationEnabled ? STATUS_SIMULATION : "Actual Testing Mode";
  }
  const simCard = document.querySelector(".metric-card--simulate");
  if (simCard) {
    simCard.classList.toggle("is-simulating", simulationEnabled);
  }
}

function setSimulationMode(enabled) {
  const shouldEnable = Boolean(enabled);
  if (simulationEnabled === shouldEnable) {
    updateSimulationToggleUi();
    if (simulationEnabled) {
      applySimulationStatusIndicator(STATUS_SIMULATION);
    }
    return simulationEnabled;
  }

  simulationEnabled = shouldEnable;
  updateSimulationToggleUi();

  if (simulationStatusResetTimer) {
    window.clearTimeout(simulationStatusResetTimer);
    simulationStatusResetTimer = 0;
  }
  if (simulationRestoreTimer) {
    window.clearTimeout(simulationRestoreTimer);
    simulationRestoreTimer = 0;
  }

  if (simulationEnabled) {
    setConnectionStatus(true, "simulation");
    applySimulationStatusIndicator(STATUS_SIMULATION);
    mockStream();
    if (!simulationIntervalId) {
      simulationIntervalId = window.setInterval(mockStream, 1500);
    }
    return true;
  }

  if (simulationIntervalId) {
    window.clearInterval(simulationIntervalId);
    simulationIntervalId = null;
  }

  // Always transition back to the original/default pose when simulation is turned off.
  clearDashboardData("manual");
  setConnectionStatus(false, "simulation");

  const restoreConnected = backendConnectionState && hasRealTelemetry;
  if (restoreConnected) {
    const restoreDelayMs = Math.max(CONNECTION_TRANSITION_MS + 80, 300);
    simulationRestoreTimer = window.setTimeout(() => {
      simulationRestoreTimer = 0;
      if (simulationEnabled) return;
      if (!(backendConnectionState && hasRealTelemetry)) return;
      // Just restore the data - don't call setConnectionStatus which would cancel view reset
      if (lastRealDashboardPayload) {
        queueDashboardData(lastRealDashboardPayload);
      }
    }, restoreDelayMs);
  }
  return false;
}

function onWindowResize() {
  const { width, height } = getViewportSize();
  updateResponsiveUiScale();
  camera.aspect = width / height;
  camera.updateProjectionMatrix();
  renderer.setSize(width, height);
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 1.5));

  // Also resize E-Load renderer
  if (eloadRenderer && eloadCamera) {
    eloadCamera.aspect = width / height;
    eloadCamera.updateProjectionMatrix();
    eloadRenderer.setSize(width, height);
    eloadRenderer.setPixelRatio(Math.min(window.devicePixelRatio, 1.5));
  }

  sliderInputs.forEach((input) => updateSliderUI(input));
  if (detailPanel.classList.contains("is-visible")) {
    positionDetailPanel();
  }
}

function requestResize() {
  if (resizeRafId) return;
  resizeRafId = window.requestAnimationFrame(() => {
    resizeRafId = 0;
    onWindowResize();
  });
}

// Start Loop
populateCellGrid();
setFanMode(true);
clearDashboardData("startup");

window.addEventListener("resize", requestResize, { passive: true });
if (window.visualViewport) {
  window.visualViewport.addEventListener("resize", requestResize, { passive: true });
}

onWindowResize();

window.__bmsSetSimulation = (enabled) => setSimulationMode(Boolean(enabled));
window.__bmsStartSimulation = () => setSimulationMode(true);
window.__bmsStopSimulation = () => setSimulationMode(false);
window.__bmsGetSimulationState = () => Boolean(simulationEnabled);

setSimulationMode(false);
console.log("[BMS] Simulation toggle initialized in Actual Testing Mode.");

// --- Tab Navigation Logic ---
function switchPage(pageId) {
  if (activePageId === pageId) return;
  deselectPart();
  activePageId = pageId;

  // Lazy-init E-Load scene on first switch
  if (pageId === "eload") initEloadScene();

  // Toggle page containers
  document.querySelectorAll(".page").forEach((page) => {
    page.classList.toggle("active", page.id === `page-${pageId}`);
  });

  // Toggle tab active states
  document.querySelectorAll(".page-tab").forEach((tab) => {
    tab.classList.toggle("active", tab.dataset.page === pageId);
  });

  // Animate tab indicator
  const activeTab = document.querySelector(`.page-tab[data-page="${pageId}"]`);
  const indicator = document.getElementById("tab-indicator");
  if (activeTab && indicator) {
    indicator.style.left = `${activeTab.offsetLeft}px`;
    indicator.style.width = `${activeTab.offsetWidth}px`;
  }

  console.log(`[BMS] Switched to ${pageId} page`);
}

// Initialize tab indicator position
requestAnimationFrame(() => {
  const bmsTab = document.getElementById("tab-bms");
  const indicator = document.getElementById("tab-indicator");
  if (bmsTab && indicator) {
    indicator.style.left = `${bmsTab.offsetLeft}px`;
    indicator.style.width = `${bmsTab.offsetWidth}px`;
  }
});

// Tab click handlers
document.querySelectorAll(".page-tab").forEach((tab) => {
  tab.addEventListener("click", () => {
    const pageId = tab.dataset.page;
    if (pageId) switchPage(pageId);
  });
});

window.__bmsSwitchPage = switchPage;

// --- E-Load Simulation Mode ---
const eloadSimulateToggle = document.getElementById("eload-simulate-toggle");
const eloadSimulateModeEl = document.getElementById("eload-simulate-mode");
const eloadStatusDot = document.getElementById("eload-data-pulse");
const eloadStatusLabel = document.getElementById("eload-status-label");
let eloadSimulationEnabled = false;
let eloadSimulationIntervalId = null;

function mockEloadStream() {
  const eloadData = {
    enabled: eloadToggle.checked,
    dac: 0,
    vout: 0,
    i_set: 0,
    v: 5.0 + (Math.random() - 0.5) * 0.1,  // VSENSE ~5V USB
    s1: Math.random() * 0.002,
    s2: Math.random() * 0.002,
    s3: Math.random() * 0.002,
    s4: Math.random() * 0.002,
    v_set: 0,
    ch1: eloadChToggles[0]?.checked ?? true,
    ch2: eloadChToggles[1]?.checked ?? true,
    ch3: eloadChToggles[2]?.checked ?? true,
    ch4: eloadChToggles[3]?.checked ?? true,
  };

  updateEloadTelemetry(eloadData);
}

function setEloadSimulationMode(enabled) {
  eloadSimulationEnabled = Boolean(enabled);

  if (eloadSimulateToggle) eloadSimulateToggle.checked = eloadSimulationEnabled;
  if (eloadSimulateModeEl) {
    eloadSimulateModeEl.textContent = eloadSimulationEnabled ? "Simulation Mode" : "Actual Testing Mode";
  }

  const simCard = document.querySelector("#page-eload .metric-card--simulate");
  if (simCard) simCard.classList.toggle("is-simulating", eloadSimulationEnabled);

  // Update E-Load status indicator
  if (eloadStatusDot) {
    eloadStatusDot.className = eloadSimulationEnabled
      ? "status__dot status__dot--simulation"
      : "status__dot status__dot--waiting";
  }
  if (eloadStatusLabel) {
    eloadStatusLabel.textContent = eloadSimulationEnabled ? "Simulation Mode" : "E-Load Standby";
  }

  if (eloadSimulationEnabled) {
    mockEloadStream();
    if (!eloadSimulationIntervalId) {
      eloadSimulationIntervalId = window.setInterval(mockEloadStream, 1500);
    }
    // Trigger smooth reveal transition (solid → transparent + camera orbit)
    startEloadReveal(true);
  } else {
    if (eloadSimulationIntervalId) {
      window.clearInterval(eloadSimulationIntervalId);
      eloadSimulationIntervalId = null;
    }
    // Reset telemetry display
    updateEloadTelemetry({});
    // Reverse reveal — return to solid exterior view
    startEloadReveal(false);
  }
}

if (eloadSimulateToggle) {
  eloadSimulateToggle.addEventListener("change", (event) => {
    setEloadSimulationMode(Boolean(event.target.checked));
  });
}

window.__bmsSetEloadSimulation = (enabled) => setEloadSimulationMode(Boolean(enabled));

markBootUiReady();

// --------------- Serial Port Manager (Web Serial API) ---------------
(function initSerialManager() {
  // In PyQt WebEngine, navigator.serial may exist as a stub but doesn't work.
  // Detect PyQt mode by checking for the QWebChannel bridge or localhost serving.
  const IS_PYQT_MODE = Boolean(window.qt || window.QWebChannel || location.hostname === 'localhost');
  const WEB_SERIAL_SUPPORTED = Boolean(navigator?.serial) && !IS_PYQT_MODE;

  // --- Terminal DOM ---
  const terminalEl = document.getElementById('serial-terminal');
  const terminalLog = document.getElementById('terminal-log');
  const terminalInput = document.getElementById('terminal-input');
  const terminalSend = document.getElementById('terminal-send');
  const terminalClear = document.getElementById('terminal-clear');
  const terminalClose = document.getElementById('terminal-close');
  const terminalPortBadge = document.getElementById('terminal-port-badge');

  const MAX_LOG_LINES = 800;
  let terminalLineCount = 0;
  let activeTerminalChannel = null; // 'bms' | 'eload'

  function termLog(text, cls = 'term-line--rx') {
    if (!terminalLog) return;
    const span = document.createElement('span');
    span.className = 'term-line ' + cls;
    span.textContent = text;
    terminalLog.appendChild(span);
    terminalLineCount++;
    if (terminalLineCount > MAX_LOG_LINES) {
      const excess = terminalLineCount - MAX_LOG_LINES;
      for (let i = 0; i < excess; i++) {
        if (terminalLog.firstChild) terminalLog.removeChild(terminalLog.firstChild);
      }
      terminalLineCount = MAX_LOG_LINES;
    }
    terminalLog.scrollTop = terminalLog.scrollHeight;
  }

  function toggleTerminal(show) {
    if (!terminalEl) return;
    if (show === undefined) show = !terminalEl.classList.contains('is-open');
    terminalEl.classList.toggle('is-open', show);
    terminalEl.setAttribute('aria-hidden', String(!show));
    if (show) terminalInput?.focus();
  }

  // Close / Clear terminal
  terminalClose?.addEventListener('click', () => toggleTerminal(false));
  terminalClear?.addEventListener('click', () => {
    if (terminalLog) { terminalLog.innerHTML = ''; terminalLineCount = 0; }
  });

  // --- Per-channel serial state ---
  class SerialChannel {
    constructor(prefix) {
      this.prefix = prefix;
      this.port = null;
      this.reader = null;
      this.writer = null;
      this.readLoopActive = false;
      this.connected = false;
      this.portName = '';
      // DOM elements
      this.portSelect = document.getElementById(prefix + '-port-select');
      this.baudSelect = document.getElementById(prefix + '-baud-select');
      this.connectBtn = document.getElementById(prefix + '-serial-connect');
      this.refreshBtn = document.getElementById(prefix + '-serial-refresh');
      this.termToggle = document.getElementById(prefix + '-terminal-toggle');
      this.dotEl = document.getElementById(prefix + '-serial-dot');
      this.statusLabel = document.getElementById(prefix + '-serial-status');
      this._bindUI();
    }

    _bindUI() {
      // Only bind Web Serial connect/refresh in browser mode; PyQt mode wires these separately
      if (WEB_SERIAL_SUPPORTED) {
        this.connectBtn?.addEventListener('click', () => this._handleConnectClick());
        this.refreshBtn?.addEventListener('click', () => this._scanPorts());
      }
      this.termToggle?.addEventListener('click', () => {
        activeTerminalChannel = this.prefix;
        terminalPortBadge && (terminalPortBadge.textContent = this.portName || '--');
        toggleTerminal(true);
      });
      // Initial scan if supported
      if (WEB_SERIAL_SUPPORTED) this._scanPorts();
    }

    async _scanPorts() {
      if (!WEB_SERIAL_SUPPORTED || !this.portSelect) return;
      try {
        const ports = await navigator.serial.getPorts();
        this.portSelect.innerHTML = '';
        if (ports.length === 0) {
          const opt = document.createElement('option');
          opt.value = '';
          opt.textContent = 'Request port...';
          this.portSelect.appendChild(opt);
        } else {
          ports.forEach((p, i) => {
            const info = p.getInfo();
            const opt = document.createElement('option');
            opt.value = String(i);
            opt.textContent = info.usbVendorId
              ? 'USB (' + (info.usbVendorId.toString(16)) + ':' + (info.usbProductId?.toString(16) || '?') + ')'
              : 'Port ' + (i + 1);
            opt.dataset.portIndex = i;
            this.portSelect.appendChild(opt);
          });
        }
      } catch (err) {
        console.warn('[Serial] scan failed:', err);
      }
    }

    async _handleConnectClick() {
      if (this.connected) {
        await this.disconnect();
      } else {
        await this.connect();
      }
    }

    async connect() {
      if (!WEB_SERIAL_SUPPORTED) {
        termLog('[System] Web Serial API not supported in this browser.', 'term-line--system');
        this._updateUI(false, 'Not supported');
        return;
      }
      try {
        const baudRate = parseInt(this.baudSelect?.value || '115200', 10);
        let port;
        // Check if user has approved ports
        const knownPorts = await navigator.serial.getPorts();
        const selectedIdx = parseInt(this.portSelect?.value || '', 10);
        if (!isNaN(selectedIdx) && knownPorts[selectedIdx]) {
          port = knownPorts[selectedIdx];
        } else {
          // Request new port from user (browser security prompt)
          port = await navigator.serial.requestPort();
        }
        await port.open({ baudRate });
        this.port = port;
        this.connected = true;
        const info = port.getInfo();
        this.portName = info.usbVendorId
          ? 'USB:' + info.usbVendorId.toString(16).toUpperCase()
          : 'COM';
        this._updateUI(true, 'Connected (' + this.portName + ')');
        termLog('[System] Connected to ' + this.portName + ' @ ' + baudRate + ' baud', 'term-line--system');
        if (activeTerminalChannel === this.prefix && terminalPortBadge) {
          terminalPortBadge.textContent = this.portName;
        }
        // Start read loop
        this._startReading();
        // Rescan ports list
        await this._scanPorts();
      } catch (err) {
        console.error('[Serial] connect error:', err);
        termLog('[System] Connection failed: ' + err.message, 'term-line--system');
        this._updateUI(false, 'Connection failed');
      }
    }

    async disconnect() {
      this.readLoopActive = false;
      try {
        if (this.reader) { await this.reader.cancel(); this.reader = null; }
        if (this.writer) { this.writer.releaseLock(); this.writer = null; }
        if (this.port) { await this.port.close(); this.port = null; }
      } catch (err) {
        console.warn('[Serial] disconnect error:', err);
      }
      this.connected = false;
      this.portName = '';
      this._updateUI(false, 'Disconnected');
      termLog('[System] Disconnected', 'term-line--system');
      if (activeTerminalChannel === this.prefix && terminalPortBadge) {
        terminalPortBadge.textContent = '--';
      }
    }

    async _startReading() {
      if (!this.port?.readable) return;
      this.readLoopActive = true;
      const decoder = new TextDecoderStream();
      const readableStreamClosed = this.port.readable.pipeTo(decoder.writable);
      this.reader = decoder.readable.getReader();
      let lineBuffer = '';
      try {
        while (this.readLoopActive) {
          const { value, done } = await this.reader.read();
          if (done) break;
          if (!value) continue;
          lineBuffer += value;
          const lines = lineBuffer.split('\n');
          lineBuffer = lines.pop() || '';
          for (const line of lines) {
            const trimmed = line.replace(/\r$/, '');
            if (!trimmed) continue;
            // Log to terminal
            if (activeTerminalChannel === this.prefix) {
              termLog(trimmed, 'term-line--rx');
            }
            // Try to parse and feed into dashboard
            this._processLine(trimmed);
          }
        }
      } catch (err) {
        if (this.readLoopActive) {
          console.error('[Serial] read error:', err);
          termLog('[System] Read error: ' + err.message, 'term-line--system');
        }
      } finally {
        try { this.reader?.releaseLock(); } catch (_) { }
        try { await readableStreamClosed; } catch (_) { }
        this.reader = null;
      }
    }

    _processLine(line) {
      // Try JSON parse first
      try {
        const idx = line.indexOf('{');
        if (idx >= 0) {
          const jsonStr = line.substring(idx);
          const data = JSON.parse(jsonStr);
          if (data && typeof data === 'object') {
            if (this.prefix === 'bms') {
              window.updateDashboard?.(data);
            } else if (this.prefix === 'eload') {
              if (data.eload || data.v !== undefined || data.vsense !== undefined) {
                const eloadPayload = data.eload || data;
                window.updateDashboard?.({ eload: eloadPayload });
              }
            }
            return;  // Successfully parsed JSON, skip string parser
          }
        }
      } catch (_) {
        // Not JSON - fall through to string format parsing
      }

      // Try E-Load string format: S1=200 S2=198 S3=201 S4=199 CH1=1 CH2=1 CH3=1 CH4=1
      // Also supports legacy: I_SET=928 DAC=2048 VOUT=1750 ...
      if (this.prefix === 'eload' && line.includes('=')) {
        const pairs = {};
        const re = /(\w+)=(\d+)/g;
        let m;
        while ((m = re.exec(line)) !== null) {
          pairs[m[1].toLowerCase()] = parseInt(m[2], 10);
        }
        // Simplified format (CH1-CH4, no I_SET/DAC)
        if (pairs.ch1 !== undefined && pairs.s1 !== undefined) {
          const anyOn = pairs.ch1 || pairs.ch2 || pairs.ch3 || pairs.ch4;
          const eloadPayload = {
            i_set: 0,
            dac: 0,
            vout: 0,
            v: 0,
            s1: (pairs.s1 || 0) / 1000.0,
            s2: (pairs.s2 || 0) / 1000.0,
            s3: (pairs.s3 || 0) / 1000.0,
            s4: (pairs.s4 || 0) / 1000.0,
            enabled: Boolean(anyOn),
            v_set: 0,
            ch1: Boolean(pairs.ch1),
            ch2: Boolean(pairs.ch2),
            ch3: Boolean(pairs.ch3),
            ch4: Boolean(pairs.ch4),
          };
          window.updateDashboard?.({ eload: eloadPayload });
        }
        // Legacy format with I_SET/DAC
        else if (pairs.i_set !== undefined && pairs.dac !== undefined) {
          const eloadPayload = {
            i_set: pairs.i_set / 10.0,
            dac: pairs.dac,
            vout: (pairs.vout || 0) / 1000.0,
            v: (pairs.vsense || 0) / 1000.0,
            s1: (pairs.s1 || 0) / 1000.0,
            s2: (pairs.s2 || 0) / 1000.0,
            s3: (pairs.s3 || 0) / 1000.0,
            s4: (pairs.s4 || 0) / 1000.0,
            enabled: Boolean(pairs.en),
            v_set: (pairs.vout || 0) / 1000.0,
          };
          window.updateDashboard?.({ eload: eloadPayload });
        }
      }
    }

    async sendCommand(cmd) {
      if (!this.port?.writable) {
        termLog('[System] Not connected � cannot send', 'term-line--system');
        return;
      }
      try {
        const encoder = new TextEncoder();
        const writer = this.port.writable.getWriter();
        await writer.write(encoder.encode(cmd + '\n'));
        writer.releaseLock();
        termLog(cmd, 'term-line--tx');
      } catch (err) {
        console.error('[Serial] send error:', err);
        termLog('[System] Send failed: ' + err.message, 'term-line--system');
      }
    }

    _updateUI(connected, label) {
      this.connectBtn && (this.connectBtn.textContent = connected ? 'Disconnect' : 'Connect');
      this.connectBtn?.classList.toggle('is-connected', connected);
      this.dotEl?.classList.toggle('is-connected', connected);
      this.statusLabel && (this.statusLabel.textContent = label || (connected ? 'Connected' : 'Disconnected'));
    }
  }

  // --- Create channel instances ---
  const bmsSerial = new SerialChannel('bms');
  const eloadSerial = new SerialChannel('eload');

  // --- Terminal input send ---
  function sendTerminalCommand() {
    const cmd = terminalInput?.value?.trim();
    if (!cmd) return;
    const channel = activeTerminalChannel === 'eload' ? eloadSerial : bmsSerial;
    channel.sendCommand(cmd);
    terminalInput.value = '';
  }

  terminalSend?.addEventListener('click', sendTerminalCommand);
  terminalInput?.addEventListener('keydown', (e) => {
    if (e.key === 'Enter') { e.preventDefault(); sendTerminalCommand(); }
  });

  // --- Backend-managed serial mode (PyQt WebEngine) ---
  let _backendPortName = '';

  window.__bmsSyncSerialConfigPanel = function (connected, portName) {
    if (portName !== undefined) _backendPortName = portName || '';
    if (!connected) _backendPortName = '';
    const displayPort = _backendPortName;

    const dot = document.getElementById('bms-serial-dot');
    const label = document.getElementById('bms-serial-status');
    const connectBtn = document.getElementById('bms-serial-connect');

    if (dot) dot.classList.toggle('is-connected', connected);
    if (label) {
      label.textContent = connected && displayPort
        ? displayPort
        : connected ? 'Connected' : 'Disconnected';
    }
    if (connectBtn) {
      connectBtn.textContent = connected ? 'Disconnect' : 'Connect';
      connectBtn.classList.toggle('is-connected', connected);
    }
    // Select matching port in dropdown if connected
    if (connected && displayPort) {
      const portSelect = document.getElementById('bms-port-select');
      if (portSelect) {
        for (const opt of portSelect.options) {
          if (opt.value === displayPort) { opt.selected = true; break; }
        }
      }
    }
  };

  window.__eloadSyncSerialConfigPanel = function (connected, portName) {
    const dot = document.getElementById('eload-serial-dot');
    const label = document.getElementById('eload-serial-status');
    const connectBtn = document.getElementById('eload-serial-connect');

    if (dot) dot.classList.toggle('is-connected', connected);
    if (label) {
      label.textContent = connected && portName
        ? portName
        : connected ? 'Connected' : 'Disconnected';
    }
    if (connectBtn) {
      connectBtn.textContent = connected ? 'Disconnect' : 'Connect';
      connectBtn.classList.toggle('is-connected', connected);
    }
    if (connected && portName) {
      const portSelect = document.getElementById('eload-port-select');
      if (portSelect) {
        for (const opt of portSelect.options) {
          if (opt.value === portName) { opt.selected = true; break; }
        }
      }
    }

    // Drive the 3D reveal transition directly from the connection event.
    // This makes it reliable regardless of whether a data packet has arrived yet.
    if (connected && !eloadSimulationEnabled) {
      if (!eloadHasRevealed) {
        eloadHasRevealed = true;
        startEloadReveal(true);
      }
      // Update status indicator to show live connection
      if (eloadStatusDot) {
        eloadStatusDot.className = 'status__dot status__dot--active';
      }
      if (eloadStatusLabel) {
        eloadStatusLabel.textContent = portName ? `Live — ${portName}` : 'E-Load Live';
      }
    } else if (!connected && !eloadSimulationEnabled) {
      if (eloadHasRevealed) {
        eloadHasRevealed = false;
        startEloadReveal(false);
      }
      // Restore standby indicator
      if (eloadStatusDot) {
        eloadStatusDot.className = 'status__dot status__dot--waiting';
      }
      if (eloadStatusLabel) {
        eloadStatusLabel.textContent = 'E-Load Standby';
      }
    }
  };

  window.__bmsUpdatePortList = function (ports) {
    ['bms', 'eload'].forEach(prefix => {
      const select = document.getElementById(prefix + '-port-select');
      if (!select) return;
      const currentVal = select.value;
      select.innerHTML = '';
      if (!ports || ports.length === 0) {
        const opt = document.createElement('option');
        opt.value = '';
        opt.textContent = 'No ports found';
        select.appendChild(opt);
      } else {
        ports.forEach(p => {
          const opt = document.createElement('option');
          opt.value = p;
          opt.textContent = p;
          if (p === currentVal) opt.selected = true;
          select.appendChild(opt);
        });
      }
    });
  };

  // Disable connect button in non-Web-Serial environments immediately
  if (!WEB_SERIAL_SUPPORTED) {
    console.log('[Serial] Web Serial API not available. Using backend serial management.');

    ['bms', 'eload'].forEach(prefix => {
      const connectBtn = document.getElementById(prefix + '-serial-connect');
      const refreshBtn = document.getElementById(prefix + '-serial-refresh');

      if (refreshBtn) {
        refreshBtn.addEventListener('click', () => {
          sendBackendCommand('SERIAL:SCAN');
        });
      }

      if (connectBtn) {
        connectBtn.addEventListener('click', () => {
          const isConnected = connectBtn.classList.contains('is-connected');
          if (isConnected) {
            sendBackendCommand(`SERIAL:${prefix.toUpperCase()}:DISCONNECT`);
          } else {
            const portSelect = document.getElementById(prefix + '-port-select');
            const baudSelect = document.getElementById(prefix + '-baud-select');
            const port = portSelect?.value || '';
            const baud = baudSelect?.value || '115200';
            if (!port) {
              console.warn(`[Serial] No port selected for ${prefix}. Click refresh to scan.`);
              return;
            }
            sendBackendCommand(`SERIAL:${prefix.toUpperCase()}:CONNECT:${port}:${baud}`);
          }
        });
      }
    });

    // Trigger initial port scan after a short delay
    setTimeout(() => sendBackendCommand('SERIAL:SCAN'), 1000);
  } else {
    console.log('[Serial] Web Serial API available - serial ports can be configured from the GUI.');
  }

  // Expose for external use
  window.__bmsSerialBms = bmsSerial;
  window.__bmsSerialEload = eloadSerial;
  window.__bmsToggleTerminal = toggleTerminal;

  // Allow backend to pipe raw serial lines into the terminal
  window.__bmsTerminalAppend = function (text) {
    termLog(text, 'term-line--rx');
  };
})();
