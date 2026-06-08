#!/usr/bin/env python3
"""Generate the RDSA arm catalog YAML from vendor robot-description submodules.

Re-runnable. For each vendor, enumerates standalone urdf/<type>.urdf.xacro that
have a matching config/<type>/ dir, and writes
config/catalog/arms/<vendor>.yml (overwriting). Run from anywhere; paths are
resolved relative to this file.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

import yaml

REPO_ROOT = Path(__file__).resolve().parent.parent
DEPS = REPO_ROOT / "deps"
ARMS_OUT = (
    REPO_ROOT / "robot_description_setup_assistant" / "config" / "catalog" / "arms"
)
PLACEHOLDER_IMAGE = "resources/graphics/placeholder.png"

# vendor key -> metadata. "label" is the human prefix used in display names.
VENDORS = {
    "kuka": {
        "package": "kuka_robot_descriptions",
        "display_name": "KUKA Robots",
        "manufacturer": "KUKA Robotics",
        "website": "https://www.kuka.com",
        "label": "KUKA",
    },
    "abb": {
        "package": "abb_robot_descriptions",
        "display_name": "ABB Robots",
        "manufacturer": "ABB",
        "website": "https://new.abb.com/products/robotics",
        "label": "ABB",
    },
    "fanuc": {
        "package": "fanuc_robot_descriptions",
        "display_name": "FANUC Robots",
        "manufacturer": "FANUC",
        "website": "https://www.fanuc.com",
        "label": "FANUC",
    },
    "yaskawa": {
        "package": "yaskawa_robot_descriptions",
        "display_name": "Yaskawa Motoman",
        "manufacturer": "Yaskawa",
        "website": "https://www.motoman.com",
        "label": "Yaskawa",
    },
}


def vendor_dir(vendor: str) -> Path:
    return DEPS / VENDORS[vendor]["package"]
