import { describe, expect, it } from "vitest";
import * as THREE from "three";
import { markerToObject } from "../rvizMarkers";
import type { RvizMarker } from "../markerSocket";

function base(overrides: Partial<RvizMarker>): RvizMarker {
  return {
    ns: "a",
    id: 1,
    type: 1,
    action: 0,
    frame_id: "map",
    pose: {
      position: { x: 1, y: 2, z: 3 },
      orientation: { x: 0, y: 0, z: 0, w: 1 },
    },
    scale: { x: 1, y: 1, z: 1 },
    color: { r: 1, g: 0, b: 0, a: 1 },
    points: [],
    text: "",
    lifetime: 0,
    ...overrides,
  };
}

describe("markerToObject", () => {
  it("maps CUBE to a Mesh at the marker position", () => {
    const obj = markerToObject(base({ type: 1 }));
    expect(obj).toBeInstanceOf(THREE.Mesh);
    expect(obj!.position.x).toBe(1);
    expect(obj!.position.z).toBe(3);
  });

  it("maps SPHERE to a Mesh", () => {
    expect(markerToObject(base({ type: 2 }))).toBeInstanceOf(THREE.Mesh);
  });

  it("maps LINE_STRIP to a Line (not LineSegments)", () => {
    const obj = markerToObject(
      base({ type: 4, points: [{ x: 0, y: 0, z: 0 }, { x: 1, y: 0, z: 0 }] }),
    );
    expect(obj).toBeInstanceOf(THREE.Line);
    expect(obj!.type).toBe("Line");
  });

  it("maps LINE_LIST to LineSegments", () => {
    const obj = markerToObject(
      base({ type: 5, points: [{ x: 0, y: 0, z: 0 }, { x: 1, y: 0, z: 0 }] }),
    );
    expect(obj!.type).toBe("LineSegments");
  });

  it("maps SPHERE with non-uniform scale to a scaled Mesh", () => {
    const obj = markerToObject(base({ type: 2, scale: { x: 1, y: 2, z: 3 } }));
    expect(obj).toBeInstanceOf(THREE.Mesh);
    expect(obj!.scale.y).toBe(2);
    expect(obj!.scale.z).toBe(3);
  });

  it("applies color alpha to LINE_STRIP material", () => {
    const obj = markerToObject(
      base({ type: 4, color: { r: 1, g: 0, b: 0, a: 0.3 }, points: [{ x: 0, y: 0, z: 0 }] }),
    ) as THREE.Line;
    const mat = obj.material as THREE.LineBasicMaterial;
    expect(mat.transparent).toBe(true);
    expect(mat.opacity).toBeCloseTo(0.3);
  });

  it("returns null for unsupported types", () => {
    expect(markerToObject(base({ type: 999 }))).toBeNull();
  });
});
