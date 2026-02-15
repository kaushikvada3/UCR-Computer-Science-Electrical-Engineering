import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";
import { FBXLoader } from "three/addons/loaders/FBXLoader.js";

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

const BOOT_STAGE_WEIGHTS = {
  bootstrap: 0.10,
  modelDownload: 0.65,
  modelProcess: 0.20,
  finalize: 0.05,
};
const BOOT_REVEAL_HOLD_MS = 450;
const BOOT_REVEAL_DURATION_MS = 2400;
const STARTUP_HANDOFF_TARGET = {
  windowWidth: 1400,
  windowHeight: 900,
  chromeHeight: 64,
  focusLiftRatio: 0.06,
  yBiasRatio: 0.08,
};

const bootLoaderEl = document.getElementById("boot-loader");
const bootProgressFillEl = document.getElementById("boot-progress-fill");
const bootPercentEl = document.getElementById("boot-percent");
const bootStageEl = document.getElementById("boot-stage");
const bootDetailEl = document.getElementById("boot-detail");

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
  phase: "loading",
};
let bootRevealHoldTimer = 0;

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
  bootState.phase = "finalize-hold";
  setBootStage("finalize", "Finalizing startup...");
  setBootDetail("Preparing cinematic transition...");
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

function projectWorldPointToScreen(point, viewportWidth, viewportHeight, aspectOverride = 0) {
  if (!point || !camera) {
    return {
      x: viewportWidth * 0.5,
      y: viewportHeight * 0.5,
    };
  }

  let projectionCamera = camera;
  if (Number.isFinite(aspectOverride) && aspectOverride > 0) {
    projectionCamera = camera.clone();
    projectionCamera.aspect = aspectOverride;
    projectionCamera.updateProjectionMatrix();
    projectionCamera.position.copy(camera.position);
    projectionCamera.quaternion.copy(camera.quaternion);
    projectionCamera.updateMatrixWorld(true);
  }

  const projected = point.clone().project(projectionCamera);
  if (!Number.isFinite(projected.x) || !Number.isFinite(projected.y)) {
    return {
      x: viewportWidth * 0.5,
      y: viewportHeight * 0.5,
    };
  }

  return {
    x: ((projected.x + 1) * 0.5) * viewportWidth,
    y: ((1 - projected.y) * 0.5) * viewportHeight,
  };
}

function getModelHandoffScreenPoint() {
  const viewportWidth = Math.max(window.innerWidth, 1);
  const viewportHeight = Math.max(window.innerHeight, 1);
  const fallback = { x: viewportWidth * 0.5, y: viewportHeight * 0.5 };
  if (!loadedModel || !camera || !controls) return fallback;

  const bounds = new THREE.Box3().setFromObject(loadedModel);
  const focusPoint = bounds.isEmpty()
    ? controls.target.clone()
    : bounds.getCenter(new THREE.Vector3());
  if (!bounds.isEmpty()) {
    const size = bounds.getSize(new THREE.Vector3());
    focusPoint.y += size.y * STARTUP_HANDOFF_TARGET.focusLiftRatio;
  }

  const targetCanvasHeight = Math.max(
    STARTUP_HANDOFF_TARGET.windowHeight - STARTUP_HANDOFF_TARGET.chromeHeight,
    1,
  );
  const targetAspect = STARTUP_HANDOFF_TARGET.windowWidth / targetCanvasHeight;
  const projected = projectWorldPointToScreen(focusPoint, viewportWidth, viewportHeight, targetAspect);
  const biasedY = projected.y + (viewportHeight * STARTUP_HANDOFF_TARGET.yBiasRatio);

  return {
    x: clamp(projected.x, 0, viewportWidth),
    y: clamp(biasedY, 0, viewportHeight),
  };
}

function runRevealSequence() {
  const loaderContent = document.getElementById("boot-loader-content");
  const logoEl = document.getElementById("boot-logo");
  const loaderTitleEl = document.querySelector(".boot-loader__title");
  const loaderBarEl = document.querySelector(".boot-loader__bar");
  const loaderMetaEl = document.querySelector(".boot-loader__meta");
  const sceneCanvas = document.getElementById("scene");
  const hudRoot = document.querySelector(".hud");
  const detailPanel = document.querySelector(".detail-panel");
  const header = document.querySelector(".hud__header");

  if (!bootLoaderEl || !gsap) {
    bootState.hidden = true;
    bootState.phase = "complete";
    document.body.classList.remove("is-booting", "is-revealing");
    if (bootLoaderEl) bootLoaderEl.remove();
    return;
  }
  const leftPanels = gsap.utils.toArray(".left-column .glass-panel");
  const rightPanels = gsap.utils.toArray(".right-column .glass-panel");
  const statusEl = document.querySelector(".status");

  bootState.phase = "reveal";
  document.body.classList.add("is-revealing");
  document.body.classList.remove("is-booting");

  const revealDuration = BOOT_REVEAL_DURATION_MS / 1000;
  const handoffPoint = getModelHandoffScreenPoint();
  const logoRect = logoEl?.getBoundingClientRect() || null;
  const logoCenterX = logoRect ? (logoRect.left + (logoRect.width * 0.5)) : (window.innerWidth * 0.5);
  const logoCenterY = logoRect ? (logoRect.top + (logoRect.height * 0.5)) : (window.innerHeight * 0.5);
  const logoMoveX = handoffPoint.x - logoCenterX;
  const logoMoveY = handoffPoint.y - logoCenterY;
  const modelPulseTo = loadedModel
    ? { x: loadedModel.scale.x, y: loadedModel.scale.y, z: loadedModel.scale.z }
    : null;
  const modelPulseFrom = modelPulseTo
    ? { x: modelPulseTo.x * 0.88, y: modelPulseTo.y * 0.88, z: modelPulseTo.z * 0.88 }
    : null;

  const tl = gsap.timeline({
    defaults: { ease: "power3.out" },
    onComplete: () => {
      bootState.hidden = true;
      bootState.phase = "complete";
      document.body.classList.remove("is-revealing");
      if (bootLoaderEl) bootLoaderEl.remove();
      gsap.set(
        [
          loaderContent,
          logoEl,
          loaderTitleEl,
          bootStageEl,
          loaderBarEl,
          loaderMetaEl,
          sceneCanvas,
          hudRoot,
          detailPanel,
          header,
          ...leftPanels,
          ...rightPanels,
          statusEl,
        ].filter(Boolean),
        { clearProps: "all" },
      );
    },
  });

  tl.set([sceneCanvas, hudRoot].filter(Boolean), { visibility: "visible" }, 0);

  // Phase 1: Glass card collapses — text slides down and fades, bar shrinks
  tl.to(
    [loaderTitleEl, bootStageEl, loaderMetaEl].filter(Boolean),
    {
      opacity: 0,
      y: 18,
      duration: 0.35,
      stagger: 0.05,
      ease: "power3.in",
    },
    0,
  );

  tl.to(
    [loaderBarEl].filter(Boolean),
    {
      opacity: 0,
      scaleX: 0.3,
      duration: 0.3,
      ease: "power3.in",
    },
    0.05,
  );

  // Phase 2: Logo lifts and morphs toward the 3D model position
  if (logoEl) {
    tl.to(
      logoEl,
      {
        x: logoMoveX,
        y: logoMoveY,
        scale: 1.6,
        opacity: 0,
        duration: revealDuration * 0.55,
        ease: "expo.inOut",
      },
      0.15,
    );
  }

  // Phase 2b: Glass card scales down and dissolves
  if (loaderContent) {
    tl.to(
      loaderContent,
      {
        opacity: 0,
        scale: 0.92,
        y: 20,
        duration: 0.5,
        ease: "power2.inOut",
      },
      0.2,
    );
  }

  // Phase 3: 3D scene fades in with a soft bloom
  if (sceneCanvas) {
    tl.fromTo(
      sceneCanvas,
      { opacity: 0, filter: "brightness(1.4) blur(4px)" },
      {
        opacity: 1,
        filter: "brightness(1) blur(0px)",
        duration: revealDuration * 0.5,
        ease: "sine.out",
      },
      revealDuration * 0.22,
    );
  }

  // Phase 3b: Model scales up from slightly smaller with a satisfying pop
  if (loadedModel && modelPulseFrom && modelPulseTo) {
    tl.fromTo(
      loadedModel.scale,
      modelPulseFrom,
      {
        ...modelPulseTo,
        duration: revealDuration * 0.45,
        ease: "back.out(1.4)",
      },
      revealDuration * 0.25,
    );
  }

  // Phase 4: HUD elements enter with staggered cascading slide
  tl.add(() => {
    if (hudRoot) gsap.set(hudRoot, { visibility: "visible", opacity: 1 });
  }, revealDuration * 0.52);

  if (header) {
    tl.fromTo(
      header,
      { opacity: 0, y: -20 },
      { opacity: 1, y: 0, duration: 0.6, ease: "expo.out" },
      revealDuration * 0.54,
    );
  }

  tl.fromTo(
    leftPanels,
    { opacity: 0, x: -40, scale: 0.95 },
    {
      opacity: 1,
      x: 0,
      scale: 1,
      duration: 0.65,
      stagger: 0.09,
      ease: "expo.out",
    },
    revealDuration * 0.58,
  );

  tl.fromTo(
    rightPanels,
    { opacity: 0, x: 40, scale: 0.95 },
    {
      opacity: 1,
      x: 0,
      scale: 1,
      duration: 0.65,
      stagger: 0.09,
      ease: "expo.out",
    },
    revealDuration * 0.62,
  );

  if (statusEl) {
    tl.fromTo(
      statusEl,
      { opacity: 0, y: 16, scale: 0.9 },
      { opacity: 1, y: 0, scale: 1, duration: 0.5, ease: "back.out(2)" },
      revealDuration * 0.78,
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
const DEFAULT_CAMERA_POSITION = new THREE.Vector3(15, 12, 20);
const DEFAULT_CAMERA_TARGET = new THREE.Vector3(0, 0, 0);
camera.position.copy(DEFAULT_CAMERA_POSITION);
camera.up.set(0, 1, 0);

const controls = new OrbitControls(camera, renderer.domElement);
controls.enableDamping = true;
controls.dampingFactor = 0.05;
controls.enablePan = false;
controls.maxPolarAngle = Math.PI / 2.1;
controls.target.copy(DEFAULT_CAMERA_TARGET);
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
const meshInfos = [];
const selectedCellUuidSet = new Set();
let highlightedCellId = null;
let loadedModel = null;
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

// --- Interaction (Raycaster) ---
const raycaster = new THREE.Raycaster();
const mouse = new THREE.Vector2();

window.addEventListener('click', onMouseClick, false);

function onMouseClick(event) {
  if (event.target instanceof Element) {
    const target = event.target;
    if (target.closest(".hud") || target.closest("[data-detail-panel]")) {
      return;
    }
  }

  // Calculate mouse position in normalized device coordinates
  const { width, height } = getViewportSize();
  mouse.x = (event.clientX / width) * 2 - 1;
  mouse.y = -(event.clientY / height) * 2 + 1;

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
          cancelScheduledDetailRefresh();
          detailPendingForceGraph = false;
          detailPanel.classList.remove("is-visible");
          highlightCell(null);
        } else {
          // Show detail
          showDetail(cellEntry.id);
          highlightCell(cellEntry.id);
        }
      } else {
        // Clicked something else (frame, etc)
        cancelScheduledDetailRefresh();
        detailPendingForceGraph = false;
        highlightCell(null);
        document.querySelector("[data-detail-panel]").classList.remove("is-visible");
      }
    }
  }
}


// --- UI & Data Logic ---
const packVoltageEl = document.querySelector("[data-pack-voltage]");
const packCurrentEl = document.querySelector("[data-pack-current]");
const packTempEl = document.querySelector("[data-pack-temp]");
const fanSpeed1El = document.querySelector("[data-fan-speed-1]");
const fanSpeed2El = document.querySelector("[data-fan-speed-2]");
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
const DETAIL_REFRESH_INTERVAL_MS = 120;
const BASE_UI_WIDTH = 1400;
const BASE_UI_HEIGHT = 860;
const MIN_UI_SCALE = 0.65;
const MAX_UI_SCALE = 1.0;
const COMPACT_LAYOUT_ENTER_WIDTH = 980;
const COMPACT_LAYOUT_EXIT_WIDTH = 1080;
const CELL_VOLTAGE_MIN = 3.2;
const CELL_VOLTAGE_MAX = 4.2;
const CELL_VOLTAGE_RED_MAX = 3.5;
const CELL_VOLTAGE_GREEN_MAX = 3.63;
const LOW_CELL_MIN_FILL_PERCENT = 10;
const CELL_COUNT = 10;

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
    eload: {
      enabled: false,
      target_voltage: 0,
      target_current: 0,
      voltage: null,
      actual_current: null,
      power: null,
    },
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
    if (isFiniteNumber(cell.temperature)) {
      detailTemp.textContent = `${cell.temperature.toFixed(1)} \u00B0C`;
    } else {
      detailTemp.textContent = "-- \u00B0C";
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

function updateHud(data) {
  const cells = Array.isArray(data.cells) ? data.cells : [];
  const validVoltageCells = cells.filter((cell) => isFiniteNumber(cell?.voltage));
  const validTemps = cells
    .map((cell) => cell?.temperature)
    .filter((temp) => isFiniteNumber(temp));
  const packCurrent = isFiniteNumber(data.pack_current) ? data.pack_current : null;

  const packVoltage = validVoltageCells.reduce((acc, cell) => acc + cell.voltage, 0);
  packVoltageEl.textContent = validVoltageCells.length ? `${packVoltage.toFixed(1)} V` : "-- V";
  if (packCurrentEl) {
    packCurrentEl.textContent = packCurrent !== null ? `${packCurrent.toFixed(3)} A` : "-- A";
  }
  packTempEl.textContent = validTemps.length ? `${Math.max(...validTemps).toFixed(1)} \u00B0C` : "-- \u00B0C";

  const fan1Rpm = parseRpmValue(data.fan1?.rpm)
    ?? parseRpmValue(data.fan1_rpm)
    ?? parseRpmValue(data.fan?.rpm)
    ?? parseRpmValue(currentState.fan1?.rpm)
    ?? 0;
  const fan2Rpm = parseRpmValue(data.fan2?.rpm)
    ?? parseRpmValue(data.fan2_rpm)
    ?? parseRpmValue(data.fan?.rpm2)
    ?? parseRpmValue(currentState.fan2?.rpm)
    ?? (fan1Rpm > 0 ? fan1Rpm : 0);
  fanSpinRpm = Math.max(fan1Rpm, fan2Rpm, 0);
  if (fanMeshes.length === 1) {
    fanMeshes[0].rpm = fanSpinRpm;
  } else if (fanMeshes.length > 1) {
    const hasDualTelemetry = fan1Rpm > 0 && fan2Rpm > 0;
    const xValues = fanMeshes
      .map((entry) => Number.isFinite(entry?.worldX) ? entry.worldX : null)
      .filter((value) => value !== null);
    const splitX = xValues.length
      ? (Math.min(...xValues) + Math.max(...xValues)) / 2
      : 0;
    fanMeshes.forEach((entry, index) => {
      if (!entry) return;
      if (hasDualTelemetry && (entry.fanId === 1 || entry.fanId === 2)) {
        entry.rpm = entry.fanId === 1 ? fan1Rpm : fan2Rpm;
        return;
      }
      if (hasDualTelemetry) {
        const x = Number.isFinite(entry.worldX) ? entry.worldX : 0;
        entry.rpm = x <= splitX ? fan1Rpm : fan2Rpm;
        return;
      }
      // If only one fan RPM is available, spin all blades with the available value.
      entry.rpm = fanSpinRpm;
    });
  }
  fanSpeed1El.textContent = fan1Rpm > 0 ? `${Math.round(fan1Rpm).toLocaleString()} RPM` : "-- RPM";
  fanSpeed2El.textContent = fan2Rpm > 0 ? `${Math.round(fan2Rpm).toLocaleString()} RPM` : "-- RPM";

  // Trigger data pulse with speed based on data rate
  // Use average fan speed as a proxy for data transmission rate
  const avgFanSpeed = (fan1Rpm + fan2Rpm) / 2;
  if (avgFanSpeed > 0) {
    triggerDataPulse(avgFanSpeed);
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
      const displayPct = cell.voltage < CELL_VOLTAGE_RED_MAX
        ? Math.max(pct, LOW_CELL_MIN_FILL_PERCENT)
        : pct;
      levelEl.style.height = `${displayPct}%`;

      // Color thresholds:
      // < 3.5V -> red, 3.5V to 3.63V -> green, > 3.63V -> orange.
      if (cell.voltage < CELL_VOLTAGE_RED_MAX) levelEl.style.backgroundColor = 'var(--danger-color)';
      else if (cell.voltage <= CELL_VOLTAGE_GREEN_MAX) levelEl.style.backgroundColor = 'var(--success-color)';
      else levelEl.style.backgroundColor = 'orange';
    }
  });

  if (detailPanel.classList.contains("is-visible") && Number.isInteger(highlightedCellId)) {
    scheduleDetailRefresh(false);
  }
}

function colorForVoltage(voltage) {
  if (!isFiniteNumber(voltage)) {
    return new THREE.Color(0.55, 0.55, 0.55);
  }
  if (voltage < CELL_VOLTAGE_RED_MAX) {
    return new THREE.Color(0xff453a); // danger
  }
  if (voltage <= CELL_VOLTAGE_GREEN_MAX) {
    return new THREE.Color(0x30d158); // success
  }
  return new THREE.Color(0xff9f0a); // high/orange
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
  updateViewResetTransition(performance.now());
  controls.update();

  // Gentle rotation of the whole model (disabled by default)
  if (AUTO_ROTATE_MODEL && loadedModel) {
    loadedModel.rotation.y += delta * 0.1; // Slow rotation
  }

  animateCells(delta);
  renderer.render(scene, camera);
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

// Initialize Channel
if (typeof QWebChannel !== "undefined" && window.qt?.webChannelTransport) {
  new QWebChannel(qt.webChannelTransport, function (channel) {
    backendLink = channel.objects.backend;
    console.log("Connected to backend bridge via QWebChannel");
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

  if (backendLink) {
    console.log("Sending to backend:", cmd);
    backendLink.sendCommand(cmd);
  } else {
    console.log("Mock Send:", cmd);
  }
}

// --- E-Load UI Logic ---
const eloadToggle = document.getElementById("eload-toggle");
const eloadVoltageSlider = document.getElementById("eload-voltage-slider");
const eloadVoltageInput = document.getElementById("eload-voltage-input");
const eloadCurrentSlider = document.getElementById("eload-current-slider");
const eloadCurrentInput = document.getElementById("eload-current-input");

// Telemetry Elements
const telemVoltage = document.getElementById("telem-voltage");
const telemCurrent = document.getElementById("telem-current");
const telemPower = document.getElementById("telem-power");
const telemRPM = document.getElementById("telem-rpm");

// Fan Elements
const fanAutoBtn = document.getElementById("fan-auto-btn");
const fanManualBtn = document.getElementById("fan-manual-btn");
const fanManualControls = document.getElementById("fan-manual-controls");
const fanSlider = document.getElementById("fan-slider");
const fanValue = document.getElementById("fan-value");
const simulateDataToggle = document.getElementById("simulate-data-toggle");
const simulateDataModeEl = document.getElementById("simulate-data-mode");

let isFanAuto = true;

// -- E-Load Control --
eloadToggle.addEventListener("change", (e) => {
  const cmd = e.target.checked ? "ELOAD:ON" : "ELOAD:OFF";
  sendBackendCommand(cmd);
});

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

function commitVoltageSetpoint(rawValue) {
  const value = normalizeSetpoint(rawValue, 0, 40);
  syncSetpointControl(eloadVoltageSlider, eloadVoltageInput, value);
  sendBackendCommand(`ELOAD:VSET:${value.toFixed(2)}`);
}

function commitCurrentSetpoint(rawValue) {
  const value = normalizeSetpoint(rawValue, 0, 20);
  syncSetpointControl(eloadCurrentSlider, eloadCurrentInput, value);
  sendBackendCommand(`ELOAD:ISET:${value.toFixed(2)}`);
  sendBackendCommand(`ELOAD:SET:${Math.round(value * 1000)}`);
}

eloadVoltageSlider.addEventListener("input", (e) => {
  const value = normalizeSetpoint(e.target.value, 0, 40);
  eloadVoltageInput.value = value.toFixed(2);
  updateSliderUI(e.target);
});

eloadVoltageSlider.addEventListener("change", (e) => {
  commitVoltageSetpoint(e.target.value);
});

eloadVoltageInput.addEventListener("change", (e) => {
  commitVoltageSetpoint(e.target.value);
});

eloadCurrentSlider.addEventListener("input", (e) => {
  const value = normalizeSetpoint(e.target.value, 0, 20);
  eloadCurrentInput.value = value.toFixed(2);
  updateSliderUI(e.target);
});

eloadCurrentSlider.addEventListener("change", (e) => {
  commitCurrentSetpoint(e.target.value);
});

eloadCurrentInput.addEventListener("change", (e) => {
  commitCurrentSetpoint(e.target.value);
});

// -- Fan Control --
fanAutoBtn.addEventListener("click", () => {
  setFanMode(true);
  sendBackendCommand("FAN:AUTO");
});

fanManualBtn.addEventListener("click", () => {
  setFanMode(false);
  sendBackendCommand("FAN:MANUAL");
});

fanSlider.addEventListener("input", (e) => {
  fanValue.textContent = `${e.target.value}%`;
  updateSliderUI(e.target);
});

fanSlider.addEventListener("change", (e) => {
  const duty = clamp(parseInt(e.target.value, 10) || 0, 0, 100);
  fanSlider.value = duty.toString();
  fanValue.textContent = `${duty}%`;
  updateSliderUI(fanSlider);
  sendBackendCommand(`FAN:SET:${duty}`);
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
  telemVoltage.textContent = isFiniteNumber(eload?.voltage) ? `${eload.voltage.toFixed(2)} V` : "-- V";
  telemCurrent.textContent = isFiniteNumber(eload?.actual_current) ? `${eload.actual_current.toFixed(2)} A` : "-- A";
  telemPower.textContent = isFiniteNumber(eload?.power) ? `${eload.power.toFixed(1)} W` : "-- W";
  const fanRpm = isFiniteNumber(currentState.fan1?.rpm) ? Number(currentState.fan1.rpm) : 0;
  telemRPM.textContent = fanRpm > 0 ? `${Math.round(fanRpm)} RPM` : "--";

  if (typeof eload?.enabled === "boolean") {
    eloadToggle.checked = eload.enabled;
  }

  const activeElement = document.activeElement;
  if (
    activeElement !== eloadVoltageInput &&
    activeElement !== eloadVoltageSlider &&
    isFiniteNumber(eload?.target_voltage)
  ) {
    syncSetpointControl(eloadVoltageSlider, eloadVoltageInput, eload.target_voltage);
  }
  if (
    activeElement !== eloadCurrentInput &&
    activeElement !== eloadCurrentSlider &&
    isFiniteNumber(eload?.target_current)
  ) {
    syncSetpointControl(eloadCurrentSlider, eloadCurrentInput, eload.target_current);
  }
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
  clearMeshTargets();

  updateHud(currentState);
  updateEloadTelemetry(currentState.eload);
  syncSetpointControl(eloadVoltageSlider, eloadVoltageInput, 0);
  syncSetpointControl(eloadCurrentSlider, eloadCurrentInput, 0);
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
  if (data.eload) {
    currentState.eload = {
      enabled: Boolean(data.eload.enabled),
      target_voltage: isFiniteNumber(data.eload.target_voltage)
        ? Number(data.eload.target_voltage)
        : currentState.eload?.target_voltage || 0,
      target_current: isFiniteNumber(data.eload.target_current)
        ? Number(data.eload.target_current)
        : currentState.eload?.target_current || 0,
      voltage: isFiniteNumber(data.eload.voltage) ? Number(data.eload.voltage) : null,
      actual_current: isFiniteNumber(data.eload.actual_current) ? Number(data.eload.actual_current) : null,
      power: isFiniteNumber(data.eload.power) ? Number(data.eload.power) : null,
    };
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

  window.updateDashboard({
    __simulated: true,
    cells,
    pack_current: mockCurrent,
    fan1: { rpm: 900 + Math.round(Math.random() * 500) },
    fan2: { rpm: 900 + Math.round(Math.random() * 500) },
    fan_control: {
      auto: isFanAuto,
      duty: parseInt(fanSlider.value, 10) || 0,
    },
    eload: {
      enabled: eloadToggle.checked,
      target_voltage: parseFloat(eloadVoltageInput.value) || 0,
      target_current: parseFloat(eloadCurrentInput.value) || 0,
      voltage: mockVoltage,
      actual_current: mockActualCurrent,
      power: Number((mockVoltage * mockActualCurrent).toFixed(2)),
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
markBootUiReady();
