import { describe, expect, it, vi } from "vitest";
import {
  fetchCategories,
  fetchUrdf,
  fetchValidation,
  filterRobots,
  imageUrl,
} from "../api";

describe("fetchUrdf", () => {
  it("returns the parsed URDF response", async () => {
    const fakeFetch = vi.fn().mockResolvedValue({
      ok: true,
      json: async () => ({
        urdf_xml: "<robot/>",
        mesh_base: "/meshes",
        missing_packages: [],
      }),
    });
    const result = await fetchUrdf("ur3", fakeFetch as unknown as typeof fetch);
    expect(fakeFetch).toHaveBeenCalledWith("/api/robots/ur3/urdf");
    expect(result.urdf_xml).toBe("<robot/>");
    expect(result.mesh_base).toBe("/meshes");
  });

  it("throws on a non-ok response", async () => {
    const fakeFetch = vi.fn().mockResolvedValue({ ok: false, status: 404 });
    await expect(
      fetchUrdf("nope", fakeFetch as unknown as typeof fetch),
    ).rejects.toThrow("404");
  });
});

describe("catalog API", () => {
  it("posts a filter and returns robots", async () => {
    const fakeFetch = vi.fn().mockResolvedValue({
      ok: true,
      json: async () => [{ id: "ur3", display_name: "UR3", category: "universal_robots" }],
    });
    const result = await filterRobots(
      { category: "universal_robots" },
      fakeFetch as unknown as typeof fetch,
    );
    expect(fakeFetch).toHaveBeenCalledWith("/api/robots/filter", {
      method: "POST",
      headers: { "content-type": "application/json" },
      body: JSON.stringify({ category: "universal_robots" }),
    });
    expect(result[0].id).toBe("ur3");
  });

  it("fetches categories", async () => {
    const fakeFetch = vi.fn().mockResolvedValue({
      ok: true,
      json: async () => [{ id: "kuka", display_name: "KUKA Robots" }],
    });
    const cats = await fetchCategories(fakeFetch as unknown as typeof fetch);
    expect(fakeFetch).toHaveBeenCalledWith("/api/categories");
    expect(cats[0].display_name).toBe("KUKA Robots");
  });

  it("fetches validation result", async () => {
    const fakeFetch = vi.fn().mockResolvedValue({
      ok: true,
      json: async () => ({ ok: false, missing_packages: ["ur_msgs"] }),
    });
    const v = await fetchValidation("ur3", fakeFetch as unknown as typeof fetch);
    expect(fakeFetch).toHaveBeenCalledWith("/api/robots/ur3/validate");
    expect(v.missing_packages).toEqual(["ur_msgs"]);
  });

  it("builds an image URL", () => {
    expect(imageUrl("ur3")).toBe("/api/robots/ur3/image");
  });
});
