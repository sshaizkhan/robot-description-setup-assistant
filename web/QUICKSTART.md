# Quick start — see the web UI

Goal: open the Robot Description Setup Assistant in your browser.

## Prerequisites (once)

- ROS 2 Humble installed.
- Node.js + npm.
- `python3` (or `uv`) for the backend — `web/run.sh` creates the venv for you.

## 1. Build + source the workspace (once, or after pulling)

The backend finds `robots.yaml` and the robot meshes through the ROS package
index, so the workspace must be built and sourced.

```bash
cd /home/bot/rds_ws
source /opt/ros/humble/setup.bash
colcon build --merge-install --packages-select robot_description_setup_assistant
source install/setup.bash
```

(The robot description packages under `deps/` — `ur_description`, the KUKA
packages — must also be built/sourced so their meshes resolve. In this
workspace they already are.)

## 2. Run it (one command)

```bash
cd /home/bot/rds_ws/src/robot-description-setup-assistant
web/run.sh
```

`web/run.sh`:
1. creates the Python venv + installs the backend (first run only),
2. builds the React frontend,
3. starts one server that serves both the API and the page.

Then open **http://127.0.0.1:8000**.

### See it from another machine on your network

```bash
web/run.sh 0.0.0.0 8000
```

Open `http://<this-machine-ip>:8000` from your laptop/phone (same LAN or
Tailscale). Find the IP with `hostname -I`.

### Stop the server

`Ctrl-C` in the terminal, or from elsewhere:

```bash
pkill -f "[u]vicorn rdsa_web"
```

## What you should see

Boot animation → start screen → **Browse robots** → a grid of robot cards.
Click a card to open the 3D viewer (orbit, pose the joints, randomize,
toggle collision). The right panel shows specs and a **Download bringup
package** button. Top-right: dark-mode toggle and a **Live (ROS)** switch.

## Live (ROS) mode (optional)

The viewer poses the robot from the sliders by default — no ROS needed.
Flip **Live (ROS)** to drive it from a running ROS graph publishing
`/joint_states`. To test it, in another sourced terminal:

```bash
ros2 topic pub -r 10 /joint_states sensor_msgs/msg/JointState \
  '{name: ["shoulder_pan_joint","shoulder_lift_joint","elbow_joint","wrist_1_joint","wrist_2_joint","wrist_3_joint"],
    position: [1.0, -1.2, 1.4, 0.0, 1.0, 0.0]}'
```

(Joint names must match the selected robot; the example is for a UR arm.)

## Dev mode (hot-reload, optional — two terminals)

```bash
# terminal 1 — backend API
cd web/backend && . .venv/bin/activate
RDSA_ROBOTS_YAML="$PWD/../../robot_description_setup_assistant/config/robots.yaml" \
  uvicorn rdsa_web.app:app --port 8000

# terminal 2 — frontend with hot reload (proxies /api, /meshes, /ws to :8000)
cd web/frontend && npm run dev
```

Open the URL Vite prints (usually http://127.0.0.1:5173).

## Tests

```bash
cd web/backend && . .venv/bin/activate && pytest      # backend
cd web/frontend && npm test                            # frontend
```
