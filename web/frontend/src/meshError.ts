/**
 * Extract a human-readable mesh file name from a mesh URL, for reporting which
 * meshes failed to load (e.g. a 413 when the asset exceeds the server size cap).
 *
 * "/meshes/fanuc_robot_descriptions/meshes/lr_mate_200id_4s/visual/link_1.dae"
 *   -> "link_1.dae"
 * Query strings and hash fragments are stripped. A URL with no path segment
 * falls back to the original string.
 */
export function meshNameFromUrl(url: string): string {
  const noQuery = url.split(/[?#]/)[0];
  const segments = noQuery.split("/").filter(Boolean);
  return segments.length > 0 ? segments[segments.length - 1] : url;
}
