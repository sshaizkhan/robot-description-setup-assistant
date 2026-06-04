const PACKAGE_PREFIX = "package://";

/**
 * Convert a `package://PKG/rel/path` URL into a backend mesh URL
 * (`<meshBase>/PKG/rel/path`). Non-package URLs are returned unchanged.
 */
export function resolvePackageUrl(meshBase: string, url: string): string {
  if (!url.startsWith(PACKAGE_PREFIX)) return url;
  const base = meshBase.replace(/\/$/, "");
  const rest = url.slice(PACKAGE_PREFIX.length);
  return `${base}/${rest}`;
}
