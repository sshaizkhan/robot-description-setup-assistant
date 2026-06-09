import * as THREE from "three";
import type { RvizMarker } from "./markerSocket";

const ARROW = 0;
const CUBE = 1;
const SPHERE = 2;
const LINE_STRIP = 4;
const LINE_LIST = 5;
const POINTS = 8;
const TEXT = 9;

function material(m: RvizMarker): THREE.MeshStandardMaterial {
  const c = m.color;
  const mat = new THREE.MeshStandardMaterial({
    color: new THREE.Color(c.r, c.g, c.b),
  });
  mat.transparent = c.a < 1;
  mat.opacity = c.a;
  return mat;
}

function applyPose(obj: THREE.Object3D, m: RvizMarker): void {
  const p = m.pose.position;
  const q = m.pose.orientation;
  obj.position.set(p.x, p.y, p.z);
  obj.quaternion.set(q.x, q.y, q.z, q.w);
}

function linePositions(m: RvizMarker): THREE.Vector3[] {
  return m.points.map((p) => new THREE.Vector3(p.x, p.y, p.z));
}

/**
 * Convert one RViz marker to a three.js object, or null for unsupported types.
 * Caller is responsible for adding/removing and disposing the object.
 */
export function markerToObject(m: RvizMarker): THREE.Object3D | null {
  switch (m.type) {
    case CUBE:
    case ARROW: {
      const geo = new THREE.BoxGeometry(m.scale.x, m.scale.y, m.scale.z);
      const obj = new THREE.Mesh(geo, material(m));
      applyPose(obj, m);
      return obj;
    }
    case SPHERE: {
      const geo = new THREE.SphereGeometry(m.scale.x * 0.5, 16, 12);
      const obj = new THREE.Mesh(geo, material(m));
      applyPose(obj, m);
      return obj;
    }
    case LINE_STRIP:
    case LINE_LIST: {
      const geo = new THREE.BufferGeometry().setFromPoints(linePositions(m));
      const mat = new THREE.LineBasicMaterial({
        color: new THREE.Color(m.color.r, m.color.g, m.color.b),
      });
      const obj =
        m.type === LINE_LIST
          ? new THREE.LineSegments(geo, mat)
          : new THREE.Line(geo, mat);
      applyPose(obj, m);
      return obj;
    }
    case POINTS: {
      const geo = new THREE.BufferGeometry().setFromPoints(linePositions(m));
      const mat = new THREE.PointsMaterial({
        color: new THREE.Color(m.color.r, m.color.g, m.color.b),
        size: m.scale.x || 0.02,
      });
      const obj = new THREE.Points(geo, mat);
      applyPose(obj, m);
      return obj;
    }
    case TEXT: {
      // Minimal: a small box placeholder carrying the text in userData.
      // Full text-sprite rendering is deferred; this keeps the type non-fatal.
      const geo = new THREE.BoxGeometry(0.02, 0.02, 0.02);
      const obj = new THREE.Mesh(geo, material(m));
      obj.userData.text = m.text;
      applyPose(obj, m);
      return obj;
    }
    default:
      return null;
  }
}
