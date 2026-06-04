import { describe, expect, it } from "vitest";
import { resolvePackageUrl } from "../meshUrl";

describe("resolvePackageUrl", () => {
  it("maps a package:// URL to the mesh base route", () => {
    expect(
      resolvePackageUrl("/meshes", "package://ur_description/meshes/ur3/base.dae"),
    ).toBe("/meshes/ur_description/meshes/ur3/base.dae");
  });

  it("passes through non-package URLs unchanged", () => {
    expect(resolvePackageUrl("/meshes", "file:///abs/x.stl")).toBe(
      "file:///abs/x.stl",
    );
  });

  it("handles a trailing slash on the mesh base", () => {
    expect(
      resolvePackageUrl("/meshes/", "package://pkg/a/b.stl"),
    ).toBe("/meshes/pkg/a/b.stl");
  });
});
