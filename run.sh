#!/usr/bin/env bash
# Run the whole RDSA web stack with one command:
#   1. C++ catalog node (robot_catalog_server) — owns URDF/mesh/validation
#   2. uvicorn (FastAPI, RDSA_CATALOG_BACKEND=cpp) — thin relay + serves the SPA
#   3. the React frontend is built to static files and served by uvicorn
#      (there is no separate frontend dev server in this setup)
#
# Usage: ./run.sh [host] [port]
#   host default 127.0.0.1 (use 0.0.0.0 to expose on the LAN)
#   port default 8000
#
# Env toggles:
#   BUILD_FRONTEND=1   force a frontend rebuild even if dist/ exists
#   ROS_SETUP=...      path to ROS setup.bash (default /opt/ros/humble/setup.bash)
#
# This script does NOT run `colcon build` (low-RAM machine). Build the C++
# packages once yourself:
#   colcon build --merge-install --parallel-workers 1 \
#     --packages-select robot_catalog_msgs robot_catalog_core robot_catalog_server
#
# Note: `set -u` is intentionally NOT used — ROS setup.bash and venv activate
# scripts reference unbound variables.
set -eo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HOST="${1:-127.0.0.1}"
PORT="${2:-8000}"
WS="$(cd "$HERE/../.." && pwd)"          # repo is <ws>/src/<repo>; <ws> is two up
ROS_SETUP="${ROS_SETUP:-/opt/ros/humble/setup.bash}"
# Web app lives in its own repo, vendored under deps/ (see deps/app-robot-description-setup-assistant).
APP="$HERE/deps/app-robot-description-setup-assistant"
DIST="$APP/frontend/dist"
VENV="$APP/backend/.venv"

# --- 1. ROS environment ----------------------------------------------------
[ -f "$ROS_SETUP" ] || { echo "[run] ERROR: ROS setup not found: $ROS_SETUP"; exit 1; }
# shellcheck disable=SC1090
source "$ROS_SETUP"
if [ ! -f "$WS/install/setup.bash" ]; then
  echo "[run] ERROR: workspace not built at $WS/install"
  echo "      Build first: colcon build --merge-install --parallel-workers 1 \\"
  echo "        --packages-select robot_catalog_msgs robot_catalog_core robot_catalog_server"
  exit 1
fi
# shellcheck disable=SC1091
source "$WS/install/setup.bash"

# --- 2. backend venv (bootstrap on first run) ------------------------------
if [ ! -x "$VENV/bin/uvicorn" ]; then
  echo "[run] creating backend venv (system site-packages for rclpy/ament)..."
  # Prefer uv: stdlib `python3 -m venv` needs python3-venv/ensurepip, which is
  # not installed on this machine. uv bootstraps pip into the venv itself.
  if command -v uv >/dev/null 2>&1; then
    uv venv --system-site-packages --python 3.10 "$VENV"
    # shellcheck disable=SC1091
    . "$VENV/bin/activate"
    uv pip install -e "$APP/backend"
  else
    python3 -m venv --system-site-packages "$VENV"
    # shellcheck disable=SC1091
    . "$VENV/bin/activate"
    # The venv's bundled pip/setuptools can be too old for PEP 660 editable
    # installs (pip >=21.3 + setuptools >=64 needed for build_editable).
    python -m pip install -U pip setuptools wheel
    pip install -e "$APP/backend"
  fi
else
  # shellcheck disable=SC1091
  . "$VENV/bin/activate"
fi

# --- 3. frontend build (static; served by uvicorn) -------------------------
if [ ! -f "$DIST/index.html" ] || [ "${BUILD_FRONTEND:-0}" = "1" ]; then
  command -v npm >/dev/null 2>&1 || {
    echo "[run] ERROR: npm not found and no prebuilt dist at $DIST"; exit 1; }
  echo "[run] building frontend..."
  ( cd "$APP/frontend" && npm install && npm run build )
else
  echo "[run] frontend dist present — skipping build (BUILD_FRONTEND=1 to force)"
fi

# --- 3.5 optimized meshes (GLB bundle for the web viewer) -------------------
# Download the prebuilt Draco GLB bundle (keyed to the description submodule
# commits), or generate it locally as an offline fallback, so large robots
# render. ROS-only users can skip with SKIP_MESH_BUNDLE=1.
if [ "${SKIP_MESH_BUNDLE:-0}" != "1" ]; then
  bash "$HERE/scripts/mesh-bundle.sh" fetch \
    || echo "[run] mesh bundle unavailable — large robots may not display" \
            "(SKIP_MESH_BUNDLE=1 to silence)"
fi

# --- 4. shutdown handling --------------------------------------------------
NODE_PID=""
UVI_PID=""
cleanup() {
  trap - INT TERM EXIT          # avoid re-entry
  echo
  echo "[run] shutting down..."
  [ -n "$UVI_PID" ]  && kill "$UVI_PID" 2>/dev/null || true
  # The node is launched via `setsid`, so it leads its own process group;
  # signal the whole group (the ros2 wrapper AND the node binary it spawns).
  [ -n "$NODE_PID" ] && kill -TERM "-$NODE_PID" 2>/dev/null || true
  wait 2>/dev/null || true
}
trap cleanup INT TERM EXIT

# --- 5. C++ catalog node (own process group, so we can kill it cleanly) -----
echo "[run] starting C++ catalog node (robot_catalog_server)..."
setsid ros2 run robot_catalog_server robot_catalog_server &
NODE_PID=$!

# --- 6. wait for the catalog services to come up ---------------------------
echo "[run] waiting for catalog services..."
up=0
for _ in $(seq 1 30); do
  if ros2 service list 2>/dev/null | grep -q "/catalog/get_robots"; then
    up=1; echo "[run] catalog services up"; break
  fi
  if ! kill -0 "$NODE_PID" 2>/dev/null; then
    echo "[run] ERROR: catalog node exited during startup"; exit 1
  fi
  sleep 1
done
[ "$up" = 1 ] || { echo "[run] ERROR: catalog services never appeared"; exit 1; }

# --- 7. uvicorn: relay + SPA, backed by the C++ node -----------------------
export RDSA_CATALOG_BACKEND=cpp
export RDSA_ROBOTS_YAML="$WS/install/share/robot_description_setup_assistant/config/catalog"
export RDSA_FRONTEND_DIST="$DIST"
export RDSA_MESH_OPT_DIR="$HERE/meshes-opt"   # prebuilt GLBs served at /meshes-opt

echo "[run] starting backend (cpp mode) on http://$HOST:$PORT"
echo "[run]   open:  http://${HOST}:${PORT}"
[ "$HOST" = "0.0.0.0" ] && echo "[run]   (also reachable on this machine's LAN/Tailscale IPs)"
echo "[run] Ctrl-C to stop everything."

cd "$APP/backend"
uvicorn rdsa_web.app:app --host "$HOST" --port "$PORT" &
UVI_PID=$!
wait "$UVI_PID"
