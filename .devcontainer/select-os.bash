#!/usr/bin/env bash
# Runs on the host (devcontainer initializeCommand) before compose build.
# Symlinks docker-compose.override.yml to the correct OS-specific file.
set -euo pipefail
cd "$(dirname "$0")"

case "$(uname -s)" in
  Linux)  target=docker-compose.linux.yml ;;
  Darwin) target=docker-compose.mac.yml ;;
  *)      target=docker-compose.mac.yml ;;  # safe default: no host hardware assumptions
esac

ln -sf "$target" docker-compose.override.yml
echo "select-os: docker-compose.override.yml -> $target"
