#!/bin/bash

#update and upgrade any installed packages
sudo apt update
sudo apt -y upgrade

# Install terminator
sudo apt-get install -y terminator

#install nano
sudo apt-get -y install nano


colcon --log-base "./colcon-log" mixin add default "file://`pwd`//rds_ws/src/robot-description-setup-assistant/.devcontainer/colcon_mixins/index.yaml"

sudo chown -R bot:bot /home/bot

ln -s /home/bot/rds_ws/src/robot-description-setup-assistant/.devcontainer/.vscode/ /home/bot/.vscode

# link the container settings
mkdir -p /home/bot/.vscode-server/data/Machine
ln -s /home/bot/rds_ws/src/robot-description-setup-assistant/.devcontainer/settings.json /home/bot/.vscode-server/data/Machine/settings.json

# install Terminator config
mkdir -p /home/bot/.config/terminator
ln -s /home/bot/rds_ws/src/robot-description-setup-assistant/.devcontainer/terminator.config /home/bot/.config/terminator/config

# link .bashrc
rm /home/bot/.bashrc
ln -s /home/bot/rds_ws/src/robot-description-setup-assistant/.devcontainer/.bashrc /home/bot/.bashrc

# link .clangd
rm /home/bot/.clangd
ln -s /home/bot/rds_ws/src/robot-description-setup-assistant/.devcontainer/.clangd /home/bot/.clangd

# link .bash_aliases
# rm /home/bot/.bash_aliases
ln -s /home/bot/rds_ws/src/robot-description-setup-assistant/.devcontainer/.bash_aliases /home/bot/.bash_aliases

# ---------------------------------------------------------------------------
# TigerVNC: second display :3 (port 5903), separate from host :0
#   - localhost-only bind  -> reach ONLY via SSH tunnel
#   - VncAuth password (random per rebuild) -> macOS Screen Sharing compatible
#   - openbox as window manager
#   Connect:  ssh -L 5903:127.0.0.1:5903 bot@<host>
#             then macOS Screen Sharing / viewer -> localhost:5903  (pw below)
#   Password: printed here AND saved to ~/.vnc/last_password.txt
#   Run apps: DISPLAY=:3 rviz2
# ---------------------------------------------------------------------------
VNC_DISPLAY=3
mkdir -p /home/bot/.vnc

cat > /home/bot/.vnc/xstartup <<'XEOF'
#!/bin/sh
unset SESSION_MANAGER
unset DBUS_SESSION_BUS_ADDRESS
exec openbox-session
XEOF
chmod +x /home/bot/.vnc/xstartup

cat > /home/bot/.vnc/config <<'CEOF'
geometry=1920x1080
depth=24
localhost
SecurityTypes=VncAuth
CEOF

# Generate random 8-char VNC password (VNC effective max = 8 chars)
VNC_PW=$(tr -dc 'A-Za-z0-9' </dev/urandom | head -c8)
printf '%s\n%s\nn\n' "$VNC_PW" "$VNC_PW" | vncpasswd >/dev/null 2>&1
chmod 600 /home/bot/.vnc/passwd
echo "$VNC_PW" > /home/bot/.vnc/last_password.txt
chmod 600 /home/bot/.vnc/last_password.txt
echo "==================================================="
echo " VNC display :${VNC_DISPLAY}  password: $VNC_PW"
echo " (also saved -> ~/.vnc/last_password.txt)"
echo "==================================================="

# Start it (kill stale instance on same display first; ignore errors on fresh container)
tigervncserver -kill :${VNC_DISPLAY} >/dev/null 2>&1 || true
rm -f /tmp/.X11-unix/X${VNC_DISPLAY} /tmp/.X${VNC_DISPLAY}-lock 2>/dev/null || true
tigervncserver :${VNC_DISPLAY} || echo "VNC start failed (run 'tigervncserver :${VNC_DISPLAY}' manually)"

# Check if $DISTRO=WSL then change permissions for /dev/dri/card0 and /dev/dri/renderD128
# This is required for running Gazebo in WSL
if [ "$DISTRO" = "WSL" ]; then
    sudo chmod 666 /dev/dri/card0
    sudo chmod 666 /dev/dri/renderD128
fi
