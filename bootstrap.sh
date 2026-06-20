#!/usr/bin/env bash
# One-time setup for a fresh clone of robot-description-setup-assistant.
#
# Why this exists: the robot description submodules store *.dae gzip-compressed
# behind a custom 'daegz' git filter (marked *required*). Without that filter
# configured first, a plain `git clone --recurse-submodules` FAILS or leaves the
# gzipped blobs in the tree. This script registers the filter, initializes the
# submodules so the .dae decompress to real COLLADA, and (optionally) builds the
# ROS workspace.
#
# Usage:
#   git clone https://github.com/sshaizkhan/robot-description-setup-assistant.git
#   cd robot-description-setup-assistant
#   ./bootstrap.sh           # filter + submodules
#   BUILD=1 ./bootstrap.sh   # also colcon build (low-RAM friendly)
#   ./run.sh                 # then start the app
set -eo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$HERE"
WS="$(cd "$HERE/../.." && pwd)"   # repo lives at <ws>/src/<repo>
ROS_SETUP="${ROS_SETUP:-/opt/ros/humble/setup.bash}"

# Submodules whose *.dae are stored gzipped behind the daegz filter.
DAEGZ_SUBMODULES=(abb fanuc kuka yaskawa)

is_xml() { case "$(head -c 16 "$1" 2>/dev/null)" in *'<?xml'*) return 0 ;; *) return 1 ;; esac; }

# Register the daegz filter in a repo. $1 = "" for the current repo's global
# config (--global), or a submodule path to set it locally there.
set_daegz() {
  if [ -z "$1" ]; then
    git config --global filter.daegz.clean  "gzip -n -9"
    git config --global filter.daegz.smudge "gzip -d"
    git config --global filter.daegz.required true
  else
    git -C "$1" config filter.daegz.clean  "gzip -n -9"
    git -C "$1" config filter.daegz.smudge "gzip -d"
    git -C "$1" config filter.daegz.required true
  fi
}

echo "[bootstrap] registering daegz git filter (global; only affects daegz-tagged files)…"
set_daegz ""

echo "[bootstrap] initializing submodules…"
git submodule sync --recursive
git submodule update --init --recursive

# Make sure each daegz submodule's .dae are decompressed. Handles both a fresh
# clone (filter now applies) and a prior bad clone (re-materialize gzipped ones).
for s in "${DAEGZ_SUBMODULES[@]}"; do
  d="deps/${s}_robot_descriptions"
  [ -e "$d/.git" ] || continue
  set_daegz "$d"
  sample="$(find "$d" -name '*.dae' -print -quit 2>/dev/null || true)"
  [ -n "$sample" ] || continue
  if ! is_xml "$sample"; then
    echo "[bootstrap] re-materializing gzipped .dae in $d…"
    find "$d" -name '*.dae' -delete
    git -C "$d" checkout -- .
  fi
done

sample="$(find deps/abb_robot_descriptions -name '*.dae' -print -quit 2>/dev/null || true)"
if [ -n "$sample" ] && is_xml "$sample"; then
  echo "[bootstrap] OK: .dae meshes are real COLLADA."
else
  echo "[bootstrap] WARNING: .dae still not decompressed — check 'gzip' is installed and the daegz filter." >&2
fi

PKGS=(
  robot_catalog_msgs robot_catalog_core robot_catalog_server
  robot_description_setup_assistant
  abb_robot_descriptions fanuc_robot_descriptions kuka_robot_descriptions
  ur_description yaskawa_robot_descriptions robotiq_description
)

if [ "${BUILD:-0}" = "1" ]; then
  [ -f "$ROS_SETUP" ] || { echo "[bootstrap] ERROR: ROS not found at $ROS_SETUP (set ROS_SETUP=...)"; exit 1; }
  echo "[bootstrap] building ROS workspace at $WS (parallel-workers 1, low-RAM friendly)…"
  # shellcheck disable=SC1090
  source "$ROS_SETUP"
  ( cd "$WS" && colcon build --merge-install --symlink-install --parallel-workers 1 \
      --packages-select "${PKGS[@]}" )
  echo "[bootstrap] build done."
  cat <<EOF

[bootstrap] Next:
  source "$WS/install/setup.bash"
  ./run.sh                 # → http://127.0.0.1:8000  (auto-downloads the GLB mesh bundle)
EOF
else
  cat <<EOF

[bootstrap] Submodules ready. Next:
  1. Build the workspace (or re-run as: BUILD=1 ./bootstrap.sh):
       cd "$WS"
       source "$ROS_SETUP"
       colcon build --merge-install --symlink-install --parallel-workers 1 \\
         --packages-select ${PKGS[*]}
       source install/setup.bash
  2. Start the app (auto-downloads the prebuilt GLB mesh bundle):
       cd "$HERE" && ./run.sh
EOF
fi
