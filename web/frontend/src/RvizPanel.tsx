import { useEffect, useRef, useState } from "react";
import * as THREE from "three";
import { OrbitControls } from "three/examples/jsm/controls/OrbitControls.js";
import URDFLoader, { type URDFRobot } from "urdf-loader";
import { resolvePackageUrl } from "./meshUrl";
import { connectJointStates } from "./jointSocket";
import { connectMarkers, type RvizMarker } from "./markerSocket";
import { connectTf, type TfTransform } from "./tfSocket";
import { markerToObject } from "./rvizMarkers";

interface RvizPanelProps {
  urdfXml: string | null;
  meshBase: string;
}

function wsUrl(path: string): string {
  const proto = window.location.protocol === "https:" ? "wss" : "ws";
  return `${proto}://${window.location.host}${path}`;
}

function hasWebGL(): boolean {
  return typeof WebGLRenderingContext !== "undefined";
}

/** Dispose geometry + material(s) of an object and all descendants. */
function disposeObject3D(root: THREE.Object3D): void {
  root.traverse((obj) => {
    const withGeo = obj as THREE.Mesh | THREE.Line | THREE.Points;
    withGeo.geometry?.dispose?.();
    const mat = (withGeo as THREE.Mesh).material as
      | THREE.Material
      | THREE.Material[]
      | undefined;
    if (Array.isArray(mat)) mat.forEach((m) => m.dispose());
    else mat?.dispose();
  });
}

/** Dispose every child's GPU resources, then empty the group. */
function disposeAndClear(group: THREE.Group): void {
  disposeObject3D(group);
  group.clear();
}

export function RvizPanel({ urdfXml, meshBase }: RvizPanelProps) {
  const mountRef = useRef<HTMLDivElement | null>(null);
  const sceneRef = useRef<THREE.Scene | null>(null);
  const robotRef = useRef<URDFRobot | null>(null);
  const markerGroupRef = useRef<THREE.Group | null>(null);
  const [status, setStatus] = useState("connecting…");

  // Scene setup (skipped in jsdom / no-WebGL test env).
  useEffect(() => {
    if (!hasWebGL() || !mountRef.current) return;
    const mount = mountRef.current;
    const scene = new THREE.Scene();
    sceneRef.current = scene;
    scene.background = new THREE.Color(0x1e1e22);
    scene.add(new THREE.GridHelper(10, 20, 0x444444, 0x303030));
    scene.add(new THREE.AmbientLight(0xffffff, 0.6));
    const dir = new THREE.DirectionalLight(0xffffff, 0.8);
    dir.position.set(3, 5, 2);
    scene.add(dir);

    const markerGroup = new THREE.Group();
    scene.add(markerGroup);
    markerGroupRef.current = markerGroup;

    const camera = new THREE.PerspectiveCamera(
      50,
      mount.clientWidth / Math.max(1, mount.clientHeight),
      0.01,
      100,
    );
    camera.position.set(1.5, 1.5, 1.5);
    const renderer = new THREE.WebGLRenderer({ antialias: true });
    renderer.setSize(mount.clientWidth, mount.clientHeight);
    mount.appendChild(renderer.domElement);
    const controls = new OrbitControls(camera, renderer.domElement);

    let raf = 0;
    const loop = () => {
      controls.update();
      renderer.render(scene, camera);
      raf = requestAnimationFrame(loop);
    };
    loop();

    return () => {
      cancelAnimationFrame(raf);
      controls.dispose();
      renderer.dispose();
      renderer.domElement.remove();
      sceneRef.current = null;
      markerGroupRef.current = null;
    };
  }, []);

  // Load the URDF into the scene whenever it (or the mesh base) changes.
  // Removes the previous robot first so re-selection does not stack models.
  useEffect(() => {
    if (!hasWebGL()) return;
    const scene = sceneRef.current;
    if (!scene) return;

    const prev = robotRef.current;
    if (prev) {
      disposeObject3D(prev);
      scene.remove(prev);
      robotRef.current = null;
    }
    if (!urdfXml) return;

    const loader = new URDFLoader();
    loader.packages = (pkg: string) =>
      resolvePackageUrl(meshBase, `package://${pkg}`);
    const robot = loader.parse(urdfXml) as URDFRobot;
    // URDF uses Z-up; match the grid/world orientation used by Viewer3D.
    robot.rotation.x = -Math.PI / 2;
    robotRef.current = robot;
    scene.add(robot);

    return () => {
      if (robotRef.current) {
        disposeObject3D(robotRef.current);
        scene.remove(robotRef.current);
        robotRef.current = null;
      }
    };
  }, [urdfXml, meshBase]);

  // Live joints.
  useEffect(() => {
    if (!hasWebGL()) return;
    const close = connectJointStates(
      wsUrl("/ws/joint_states"),
      (joints) => {
        const robot = robotRef.current;
        if (!robot) return;
        for (const [name, value] of Object.entries(joints)) {
          robot.setJointValue?.(name, value);
        }
      },
    );
    return close;
  }, []);

  // Live TF (status + fixed-frame transform of robot root).
  useEffect(() => {
    if (!hasWebGL()) return;
    const close = connectTf(wsUrl("/ws/tf"), (transforms: TfTransform[]) => {
      setStatus(transforms.length ? `TF: ${transforms.length} frames` : "no TF");
    });
    return close;
  }, []);

  // Live markers.
  useEffect(() => {
    if (!hasWebGL()) return;
    const close = connectMarkers(wsUrl("/ws/markers"), (markers: RvizMarker[]) => {
      const group = markerGroupRef.current;
      if (!group) return;
      disposeAndClear(group);
      for (const m of markers) {
        const obj = markerToObject(m);
        if (obj) group.add(obj);
      }
    });
    return close;
  }, []);

  return (
    <div data-testid="rviz-panel" className="rviz-panel">
      <div data-testid="rviz-status" className="rviz-status">
        {status}
      </div>
      <div ref={mountRef} className="rviz-canvas" />
      {!urdfXml && (
        <div className="rviz-empty">No robot loaded — select a robot or start a ROS graph.</div>
      )}
    </div>
  );
}
