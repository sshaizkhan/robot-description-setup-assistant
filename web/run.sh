#!/usr/bin/env bash
# Build the frontend and serve the whole app (API + SPA) from one FastAPI server.
# Bootstraps the Python venv on first run.
#
# Usage: web/run.sh [host] [port]
#   Requires: npm, python3 (or uv), and a sourced ROS env
#   (or RDSA_ROBOTS_YAML pointing at config/robots.yaml).
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HOST="${1:-127.0.0.1}"
PORT="${2:-8000}"
VENV="$HERE/backend/.venv"

# 1. Bootstrap the backend venv (system site-packages so rclpy/xacro/ament are
#    importable) and install the package if it isn't already present.
if [ ! -x "$VENV/bin/uvicorn" ]; then
  echo "[run] creating Python venv + installing backend..."
  if command -v uv >/dev/null 2>&1; then
    uv venv --system-site-packages --python 3.10 "$VENV"
    # shellcheck disable=SC1091
    . "$VENV/bin/activate"
    uv pip install -e "$HERE/backend"
  else
    python3 -m venv --system-site-packages "$VENV"
    # shellcheck disable=SC1091
    . "$VENV/bin/activate"
    pip install -e "$HERE/backend"
  fi
else
  # shellcheck disable=SC1091
  . "$VENV/bin/activate"
fi

# 2. Build the frontend.
echo "[run] building frontend..."
( cd "$HERE/frontend" && npm install && npm run build )

# 3. Serve API + built SPA from one process.
echo "[run] starting backend on http://$HOST:$PORT ..."
cd "$HERE/backend"
export RDSA_FRONTEND_DIST="$HERE/frontend/dist"
exec uvicorn rdsa_web.app:app --host "$HOST" --port "$PORT"
