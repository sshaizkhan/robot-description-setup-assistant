#!/usr/bin/env bash
# Optimized-mesh (GLB) bundle management.
#
# The web viewer needs compact Draco-compressed .glb meshes (see
# deps/app-robot-description-setup-assistant/backend/scripts/optimize_meshes.py),
# but generating them is slow and the source COLLADA lives in the (large) robot
# description submodules. So we generate ONCE in CI and publish the result as a
# GitHub Release asset; end users just download it. The bundle is keyed to the
# description submodule commits, so a download is only re-fetched when the
# underlying meshes actually change.
#
# Subcommands:
#   hash    print the bundle id (sha of the description submodule commits)
#   fetch   ensure meshes-opt/ matches the current id: reuse, else download the
#           matching release asset, else fall back to local generation
#   build   generate meshes-opt/ locally via optimize_meshes.py (offline path)
#   pack    tar.gz meshes-opt/ -> meshes-opt.tar.gz + stamp the id (CI)
#
# Env:
#   RDSA_REPO_SLUG   owner/repo to download release assets from
#                    (default sshaizkhan/robot-description-setup-assistant)
set -eo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"   # repo root
OUT="$HERE/meshes-opt"
HASH_FILE="$OUT/.bundle-hash"
ARCHIVE="$HERE/meshes-opt.tar.gz"
CONVERTER="$HERE/deps/app-robot-description-setup-assistant/backend/scripts/optimize_meshes.py"
REPO_SLUG="${RDSA_REPO_SLUG:-sshaizkhan/robot-description-setup-assistant}"

# Submodules whose COLLADA meshes feed the GLB bundle. The bundle id is derived
# from their checked-out commits — change any of them and the id changes.
MESH_SUBMODULES=(
  deps/abb_robot_descriptions
  deps/fanuc_robot_descriptions
  deps/kuka_robot_descriptions
  deps/ur_robot_descriptions
  deps/yaskawa_robot_descriptions
  deps/ros2_robotiq_gripper
)

mesh_hash() {
  local acc="" path line sha
  for path in "${MESH_SUBMODULES[@]}"; do
    line="$(git -C "$HERE" submodule status "$path" 2>/dev/null || true)"
    # submodule status: "[ +-]<sha> <path> (...)"; strip the status marker.
    sha="$(printf '%s' "$line" | sed -E 's/^[ +-]//' | awk '{print $1}')"
    acc+="$path:${sha:-missing}"$'\n'
  done
  printf '%s' "$acc" | sha256sum | cut -c1-16
}

cmd_hash() { mesh_hash; }

current_ok() {
  [ -f "$OUT/manifest.json" ] && [ "$(cat "$HASH_FILE" 2>/dev/null)" = "$1" ]
}

cmd_build() {
  local hash="${1:-$(mesh_hash)}"
  if [ ! -f "$CONVERTER" ]; then
    echo "[mesh-bundle] converter not found ($CONVERTER) — skipping. Large" >&2
    echo "[mesh-bundle] robots may not display in the web viewer." >&2
    return 1
  fi
  command -v python3 >/dev/null 2>&1 || { echo "[mesh-bundle] python3 missing" >&2; return 1; }
  echo "[mesh-bundle] generating GLBs locally (this is slow)…"
  python3 "$CONVERTER" --out "$OUT"
  echo "$hash" > "$HASH_FILE"
}

cmd_fetch() {
  local hash tag tmp got=0
  hash="$(mesh_hash)"
  if current_ok "$hash"; then
    echo "[mesh-bundle] meshes-opt up to date ($hash)"
    return 0
  fi
  tag="meshes-$hash"
  echo "[mesh-bundle] need bundle $hash"

  tmp="$(mktemp -d)"
  trap 'rm -rf "$tmp"' RETURN

  # Prefer the gh CLI: it authenticates, so this works for a PRIVATE repo (the
  # anonymous releases/download URL 404s when the repo is private). Fall back to
  # an anonymous curl, which covers a public repo on a machine without gh.
  if command -v gh >/dev/null 2>&1 && gh auth status >/dev/null 2>&1; then
    if gh release download "$tag" --repo "$REPO_SLUG" \
         --pattern meshes-opt.tar.gz --dir "$tmp" 2>/dev/null; then
      got=1
    fi
  fi
  if [ "$got" -eq 0 ]; then
    local url="https://github.com/$REPO_SLUG/releases/download/$tag/meshes-opt.tar.gz"
    curl -fsSL "$url" -o "$tmp/meshes-opt.tar.gz" && got=1 || true
  fi

  if [ "$got" -eq 1 ]; then
    echo "[mesh-bundle] downloaded release asset; extracting…"
    rm -rf "$OUT"; mkdir -p "$OUT"
    tar -xzf "$tmp/meshes-opt.tar.gz" -C "$OUT"
    echo "$hash" > "$HASH_FILE"
    echo "[mesh-bundle] ready ($hash)"
    return 0
  fi

  echo "[mesh-bundle] no prebuilt bundle for $tag (private repo without gh auth?)"
  cmd_build "$hash" || return 1
}

cmd_pack() {
  local hash; hash="$(mesh_hash)"
  [ -d "$OUT" ] || { echo "[mesh-bundle] $OUT missing — run build first" >&2; return 1; }
  echo "$hash" > "$HASH_FILE"
  # Archive the CONTENTS of meshes-opt/ (manifest.json, .bundle-hash, <pkg>/…)
  # so it extracts straight into a meshes-opt/ dir on the consumer side.
  tar -czf "$ARCHIVE" -C "$OUT" .
  echo "[mesh-bundle] packed $ARCHIVE ($hash, $(du -h "$ARCHIVE" | cut -f1))"
}

case "${1:-}" in
  hash)  cmd_hash ;;
  fetch) cmd_fetch ;;
  build) cmd_build ;;
  pack)  cmd_pack ;;
  *) echo "usage: $0 {hash|fetch|build|pack}" >&2; exit 2 ;;
esac
