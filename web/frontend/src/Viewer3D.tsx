import { useEffect, useRef, useState } from "react";
import * as THREE from "three";
import { OrbitControls } from "three/examples/jsm/controls/OrbitControls.js";
import { RoomEnvironment } from "three/examples/jsm/environments/RoomEnvironment.js";
import URDFLoader, { type URDFRobot } from "urdf-loader";
import { resolvePackageUrl } from "./meshUrl";
import { meshNameFromUrl } from "./meshError";

type AnyObj = THREE.Object3D & {
  isURDFLink?: boolean;
  isURDFVisual?: boolean;
};

const pairKey = (a: string, b: string) => (a < b ? `${a}|${b}` : `${b}|${a}`);

/** Free GPU resources (geometries, materials, textures) under an object. */
function disposeObject3D(root: THREE.Object3D): void {
  root.traverse((obj) => {
    const mesh = obj as THREE.Mesh;
    mesh.geometry?.dispose?.();
    const material = mesh.material as THREE.Material | THREE.Material[] | undefined;
    if (!material) return;
    const mats = Array.isArray(material) ? material : [material];
    for (const mat of mats) {
      for (const key of Object.keys(mat)) {
        const value = (mat as unknown as Record<string, unknown>)[key];
        if (value && (value as { isTexture?: boolean }).isTexture) {
          (value as THREE.Texture).dispose();
        }
      }
      mat.dispose();
    }
  });
}

/** Nearest URDFLink ancestor name for an object, or "" if none. */
function owningLink(obj: THREE.Object3D): string {
  let p: THREE.Object3D | null = obj.parent;
  while (p && !(p as AnyObj).isURDFLink) p = p.parent;
  return p ? p.name : "";
}

const COMMON_TOOL_FRAMES = [
  "tool0",
  "flange",
  "tool_frame",
  "ee_link",
  "tcp",
  "gripper_mount_link",
];

/**
 * Resolve the link an end-effector should mount to. Prefers the catalog's
 * declared tool frame, then common conventional names, then the deepest link in
 * the kinematic chain (the chain tip). Returns the robot root only as a last
 * resort — better a visible best-effort than a hard-coded "tool0" that silently
 * drops the EE at the base.
 */
function findToolFrame(
  robot: URDFRobot,
  preferred?: string,
): THREE.Object3D {
  for (const name of [preferred, ...COMMON_TOOL_FRAMES]) {
    if (!name) continue;
    const obj = robot.getObjectByName(name);
    if (obj) return obj;
  }
  let tip: THREE.Object3D | null = null;
  let bestDepth = -1;
  robot.traverse((c) => {
    if (!(c as AnyObj).isURDFLink) return;
    let depth = 0;
    let p: THREE.Object3D | null = c.parent;
    while (p && p !== robot) {
      depth += 1;
      p = p.parent;
    }
    if (depth > bestDepth) {
      bestDepth = depth;
      tip = c;
    }
  });
  if (!tip) {
    console.warn("Viewer3D: no tool frame found; mounting EE at robot root");
  }
  return tip ?? robot;
}

/** Group each link's own visual meshes (by owning link name). */
function collectLinkMeshes(robot: URDFRobot): Map<string, THREE.Object3D[]> {
  const groups = new Map<string, THREE.Object3D[]>();
  robot.traverse((o) => {
    if ((o as AnyObj).isURDFVisual) {
      const name = owningLink(o);
      if (!name) return;
      const arr = groups.get(name) ?? [];
      arr.push(o);
      groups.set(name, arr);
    }
  });
  return groups;
}

/** Pairs of links directly connected by a joint (always "touch"; ignore). */
function buildAdjacency(robot: URDFRobot): Set<string> {
  const adj = new Set<string>();
  const joints = robot.joints as Record<string, THREE.Object3D>;
  for (const name of Object.keys(joints)) {
    const joint = joints[name];
    let parent: THREE.Object3D | null = joint.parent;
    while (parent && !(parent as AnyObj).isURDFLink) parent = parent.parent;
    let child: THREE.Object3D | null = null;
    joint.traverse((o) => {
      if (!child && o !== joint && (o as AnyObj).isURDFLink) child = o;
    });
    if (parent && child) adj.add(pairKey(parent.name, (child as THREE.Object3D).name));
  }
  return adj;
}

/** Count intersecting, non-adjacent link pairs using shrunk world AABBs. */
function selfCollisionCount(
  groups: Map<string, THREE.Object3D[]>,
  adj: Set<string>,
): number {
  const names = [...groups.keys()];
  const center = new THREE.Vector3();
  const size = new THREE.Vector3();
  const boxes = names.map((n) => {
    const box = new THREE.Box3();
    for (const mesh of groups.get(n)!) box.expandByObject(mesh);
    if (!box.isEmpty()) {
      box.getCenter(center);
      box.getSize(size).multiplyScalar(0.8); // shrink to offset AABB over-estimate
      box.setFromCenterAndSize(center, size);
    }
    return box;
  });
  let count = 0;
  for (let i = 0; i < names.length; i++) {
    for (let j = i + 1; j < names.length; j++) {
      if (adj.has(pairKey(names[i], names[j]))) continue;
      if (boxes[i].isEmpty() || boxes[j].isEmpty()) continue;
      if (boxes[i].intersectsBox(boxes[j])) count++;
    }
  }
  return count;
}

interface Attach {
  tool_frame?: string;
  mount_frame?: string;
  xyz?: number[];
  rpy?: number[];
}

interface AssemblyPart {
  urdfXml: string;
  attach?: Attach;
}

interface Viewer3DProps {
  urdfXml: string; // the arm
  meshBase: string;
  liveJoints?: Record<string, number> | null;
  armAttach?: Attach; // arm's tool frame (default "tool0")
  endEffector?: AssemblyPart | null; // attached to the arm's tool frame
  base?: AssemblyPart | null; // the arm mounts on top of it
}

export function Viewer3D({
  urdfXml,
  meshBase,
  liveJoints,
  armAttach,
  endEffector,
  base,
}: Viewer3DProps) {
  const mountRef = useRef<HTMLDivElement>(null);
  const robotRef = useRef<URDFRobot | null>(null);
  const [joints, setJoints] = useState<string[]>([]);
  const [values, setValues] = useState<Record<string, number>>({});
  const [showCollision, setShowCollision] = useState(false);
  // Collision geometry is loaded lazily — only after the user first asks for it.
  const [collisionEnabled, setCollisionEnabled] = useState(false);
  // True while the robot's meshes are still loading; the model is hidden until
  // every mesh is in, so it appears all at once instead of link by link.
  const [loading, setLoading] = useState(true);
  // File names of meshes that failed to load (e.g. 413 — over the server size
  // cap). Surfaced as a non-blocking banner; the rest of the robot still shows.
  const [meshErrors, setMeshErrors] = useState<string[]>([]);

  useEffect(() => {
    const mount = mountRef.current;
    if (!mount || !urdfXml) return;

    const width = mount.clientWidth || 600;
    const height = mount.clientHeight || 400;

    // Viewer stays dark regardless of the app theme.
    const scene = new THREE.Scene();
    scene.background = new THREE.Color(0x1a212c);
    scene.fog = new THREE.Fog(0x1a212c, 8, 18);

    scene.add(new THREE.HemisphereLight(0xdfe9ff, 0x3a3320, 2.0));
    scene.add(new THREE.AmbientLight(0xffffff, 0.85));
    const key = new THREE.DirectionalLight(0xffffff, 2.8);
    key.position.set(2.5, 3.5, 2);
    scene.add(key);
    const fill = new THREE.DirectionalLight(0xffe6c4, 1.2);
    fill.position.set(-2.5, 1.5, -1.5);
    scene.add(fill);
    const rim = new THREE.DirectionalLight(0xcfe0ff, 0.7);
    rim.position.set(-1, 2, -3);
    scene.add(rim);
    // Bounce from below so undersides don't fall to black.
    const under = new THREE.DirectionalLight(0xffffff, 0.4);
    under.position.set(0, -3, 0.5);
    scene.add(under);

    const grid = new THREE.GridHelper(6, 24, 0x34404f, 0x1b212b);
    (grid.material as THREE.Material).transparent = true;
    (grid.material as THREE.Material).opacity = 0.7;
    scene.add(grid);

    const camera = new THREE.PerspectiveCamera(45, width / height, 0.01, 100);
    camera.position.set(1.6, 1.15, 1.6);

    const renderer = new THREE.WebGLRenderer({ antialias: true });
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    renderer.setSize(width, height);
    renderer.toneMapping = THREE.ACESFilmicToneMapping;
    renderer.toneMappingExposure = 1.5;
    mount.appendChild(renderer.domElement);

    // Image-based lighting: metallic/standard URDF materials look flat and dull
    // without an environment to reflect. A neutral room env gives them form.
    const pmrem = new THREE.PMREMGenerator(renderer);
    const envMap = pmrem.fromScene(new RoomEnvironment(), 0.04).texture;
    scene.environment = envMap;

    const controls = new OrbitControls(camera, renderer.domElement);
    controls.enableDamping = true;
    controls.dampingFactor = 0.08;
    controls.target.set(0, 0.25, 0);
    controls.update();

    setLoading(true);
    setMeshErrors([]);

    // Reveal the whole assembly only once every mesh has finished loading, so it
    // appears at once instead of building up link by link. Each loader's
    // loadMeshCb is wrapped to count outstanding loads (more reliable than
    // LoadingManager.onLoad in this urdf-loader version).
    let pending = 0;
    // URLs of meshes that failed to load (over the size cap, network, parse).
    const failed: string[] = [];
    let parseDone = false;
    let revealed = false;
    let revealTimer = 0;

    // Everything lives in one group rotated Z-up -> Y-up; base/arm are added in
    // URDF (Z-up) coordinates, the end-effector rides on the arm's tool frame.
    const assembly = new THREE.Group();
    assembly.rotation.x = -Math.PI / 2;
    assembly.visible = false;
    scene.add(assembly);

    const reveal = () => {
      if (revealed) return;
      revealed = true;
      window.clearTimeout(revealTimer);
      assembly.visible = true;
      setLoading(false);
      if (failed.length > 0) {
        setMeshErrors(Array.from(new Set(failed.map(meshNameFromUrl))));
      }
    };
    const maybeReveal = () => {
      if (parseDone && pending === 0) reveal();
    };

    const makeLoader = (withCollision: boolean): URDFLoader => {
      const l = new URDFLoader();
      l.packages = (pkg: string) =>
        resolvePackageUrl(meshBase, `package://${pkg}`);
      l.parseCollision = withCollision;
      const def = l.loadMeshCb;
      l.loadMeshCb = (
        path: string,
        manager: THREE.LoadingManager,
        done: (mesh: THREE.Object3D, err?: Error) => void,
      ) => {
        pending += 1;
        def.call(l, path, manager, (mesh: THREE.Object3D, err?: Error) => {
          if (err) {
            console.warn(`Viewer3D: mesh failed to load: ${path}`, err);
            failed.push(path);
          }
          done(mesh, err);
          pending -= 1;
          maybeReveal();
        });
      };
      return l;
    };

    // Base (optional): on the ground; the arm sits on top at this height.
    const baseHeight = base?.attach?.xyz?.[2] ?? 0;
    if (base?.urdfXml) {
      assembly.add(makeLoader(false).parse(base.urdfXml));
    }

    // Arm: the joint-controlled robot.
    const robot = makeLoader(collisionEnabled).parse(urdfXml);
    robot.position.z = baseHeight;
    assembly.add(robot);
    robotRef.current = robot;
    const movable = Object.entries(robot.joints)
      .filter(([, j]) => j.jointType !== "fixed")
      .map(([name]) => name);
    setJoints(movable);
    setValues(Object.fromEntries(movable.map((n) => [n, 0])));

    // End-effector (optional): parented to the arm's tool frame so it moves with
    // the joints.
    if (endEffector?.urdfXml) {
      const ee = makeLoader(false).parse(endEffector.urdfXml);
      const a = endEffector.attach ?? {};
      ee.position.set(a.xyz?.[0] ?? 0, a.xyz?.[1] ?? 0, a.xyz?.[2] ?? 0);
      ee.rotation.set(a.rpy?.[0] ?? 0, a.rpy?.[1] ?? 0, a.rpy?.[2] ?? 0, "ZYX");
      const tool = findToolFrame(robot, armAttach?.tool_frame);
      tool.add(ee);
    }

    parseDone = true;
    // Safety net: reveal anyway after a few seconds, and handle parts with no meshes.
    revealTimer = window.setTimeout(reveal, 6000);
    maybeReveal();

    let raf = 0;
    const animate = () => {
      raf = requestAnimationFrame(animate);
      controls.update();
      renderer.render(scene, camera);
    };
    animate();

    return () => {
      window.clearTimeout(revealTimer);
      cancelAnimationFrame(raf);
      controls.dispose();
      scene.remove(assembly);
      disposeObject3D(assembly); // free GPU geometries/materials/textures
      (grid.geometry as THREE.BufferGeometry).dispose();
      (grid.material as THREE.Material).dispose();
      scene.environment = null;
      envMap.dispose();
      pmrem.dispose();
      renderer.dispose();
      renderer.forceContextLoss();
      mount.removeChild(renderer.domElement);
      robotRef.current = null;
    };
  }, [
    urdfXml,
    meshBase,
    collisionEnabled,
    endEffector?.urdfXml,
    base?.urdfXml,
  ]);

  useEffect(() => {
    const robot = robotRef.current;
    if (!robot) return;
    // If a robot has no collider geometry, never hide its visuals (otherwise
    // enabling "Show collision" would make it disappear entirely).
    let hasCollider = false;
    robot.traverse((child) => {
      if ((child as { isURDFCollider?: boolean }).isURDFCollider) {
        hasCollider = true;
      }
    });
    robot.traverse((child) => {
      const c = child as THREE.Object3D & {
        isURDFCollider?: boolean;
        isURDFVisual?: boolean;
      };
      if (c.isURDFCollider) c.visible = showCollision;
      if (c.isURDFVisual) c.visible = hasCollider ? !showCollision : true;
    });
  }, [showCollision, joints]);

  useEffect(() => {
    const robot = robotRef.current;
    if (!robot || !liveJoints) return;
    for (const [name, value] of Object.entries(liveJoints)) {
      robot.setJointValue(name, value);
    }
  }, [liveJoints]);

  const setJoint = (name: string, value: number) => {
    setValues((v) => ({ ...v, [name]: value }));
    robotRef.current?.setJointValue(name, value);
  };

  const sampleJoint = (robot: URDFRobot, name: string): number => {
    const joint = (robot.joints as Record<string, { limit?: { lower?: number; upper?: number } }>)[name];
    const lo = joint?.limit?.lower;
    const hi = joint?.limit?.upper;
    if (typeof lo === "number" && typeof hi === "number" && hi > lo) {
      return lo + Math.random() * (hi - lo);
    }
    return (Math.random() * 2 - 1) * Math.PI;
  };

  const applyPose = (robot: URDFRobot, pose: Record<string, number>) => {
    for (const [name, value] of Object.entries(pose)) {
      robot.setJointValue(name, value);
    }
    robot.updateMatrixWorld(true);
  };

  // Rejection-sample a random pose with no self-collision (best-effort).
  const randomizePose = () => {
    const robot = robotRef.current;
    if (!robot || joints.length === 0) return;

    const groups = collectLinkMeshes(robot);
    const adj = buildAdjacency(robot);

    let best: Record<string, number> | null = null;
    let bestScore = Infinity;
    for (let attempt = 0; attempt < 60; attempt++) {
      const pose: Record<string, number> = {};
      for (const name of joints) pose[name] = sampleJoint(robot, name);
      applyPose(robot, pose);
      const score = selfCollisionCount(groups, adj);
      if (score < bestScore) {
        bestScore = score;
        best = pose;
      }
      if (score === 0) break;
    }

    if (best) {
      applyPose(robot, best);
      setValues(best);
    }
  };

  return (
    <div className="viewer3d">
      <div className="viewer-canvas-wrap">
        <div ref={mountRef} className="viewer-canvas" />
        {loading && (
          <div className="viewer-loading-overlay">
            <span className="viewer-spinner" />
            Loading model…
          </div>
        )}
        {!loading && meshErrors.length > 0 && (
          <div className="viewer-mesh-warning" role="alert">
            ⚠ {meshErrors.length} mesh
            {meshErrors.length > 1 ? "es" : ""} too large to display:{" "}
            {meshErrors.join(", ")}
          </div>
        )}
      </div>
      <div className="viewer-controls">
        <div className="viewer-actions">
          <label className="viewer-toggle">
            <input
              type="checkbox"
              checked={showCollision}
              onChange={(e) => {
                const v = e.target.checked;
                setShowCollision(v);
                if (v) setCollisionEnabled(true); // lazy-load colliders once
              }}
            />
            <span>Show collision</span>
          </label>
          {!liveJoints && joints.length > 0 && (
            <button
              type="button"
              className="randomize-btn"
              onClick={randomizePose}
            >
              ⟲ Randomize pose
            </button>
          )}
        </div>
        {liveJoints ? (
          <p className="live-note">Live ROS joint states</p>
        ) : (
          <div className="joint-grid">
            {joints.map((name) => (
              <label className="joint-row" key={name}>
                <span className="joint-name">{name}</span>
                <input
                  type="range"
                  min={-Math.PI}
                  max={Math.PI}
                  step={0.01}
                  value={values[name] ?? 0}
                  onChange={(e) => setJoint(name, Number(e.target.value))}
                />
              </label>
            ))}
          </div>
        )}
      </div>
    </div>
  );
}
