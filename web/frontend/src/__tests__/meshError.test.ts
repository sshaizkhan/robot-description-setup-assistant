import { describe, expect, it } from "vitest";
import { meshNameFromUrl } from "../meshError";

describe("meshNameFromUrl", () => {
  it("returns the file name from a mesh URL", () => {
    expect(
      meshNameFromUrl(
        "/meshes/fanuc_robot_descriptions/meshes/lr_mate_200id_4s/visual/link_1.dae",
      ),
    ).toBe("link_1.dae");
  });

  it("strips query strings and hash fragments", () => {
    expect(meshNameFromUrl("/meshes/pkg/a/b.stl?v=2#frag")).toBe("b.stl");
  });

  it("handles a trailing slash by taking the last real segment", () => {
    expect(meshNameFromUrl("/meshes/pkg/dir/")).toBe("dir");
  });

  it("falls back to the original string when there is no path segment", () => {
    expect(meshNameFromUrl("")).toBe("");
  });
});
