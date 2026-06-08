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


def discover_types(vendor: str) -> list[str]:
    """Robot types with a standalone urdf/<type>.urdf.xacro AND a config/<type>/ dir.

    The config-dir requirement naturally excludes a vendor dispatch file such as
    kuka.urdf.xacro (no config/kuka), keeping output to per-robot descriptions.
    """
    vdir = vendor_dir(vendor)
    urdf = vdir / "urdf"
    config = vdir / "config"
    types: list[str] = []
    suffix = ".urdf.xacro"
    for f in sorted(urdf.glob("*.urdf.xacro")):
        t = f.name[: -len(suffix)]
        if (config / t).is_dir():
            types.append(t)
    return types


def scrape_dof(vendor: str, type_: str) -> int:
    """Count joints in config/<type>/joint_limits.yaml; 0 if unavailable."""
    jl = vendor_dir(vendor) / "config" / type_ / "joint_limits.yaml"
    if not jl.is_file():
        return 0
    text = jl.read_text()
    try:
        data = yaml.safe_load(text) or {}
    except yaml.YAMLError:
        data = {}
    limits = data.get("joint_limits") if isinstance(data, dict) else None
    if isinstance(limits, dict):
        return len(limits)
    return len(set(re.findall(r"joint_\d+", text)))


def titleize(type_: str) -> str:
    """irb_120_3_0_6 -> 'Irb 120 3 0 6' (underscores to spaces, capitalized)."""
    return " ".join(part.capitalize() for part in type_.split("_"))


def build_entry(vendor: str, type_: str) -> dict:
    meta = VENDORS[vendor]
    return {
        "type": "arm",
        "display_name": f"{meta['label']} {titleize(type_)}",
        "description": f"{meta['label']} industrial robot",
        "image_path": PLACEHOLDER_IMAGE,
        "urdf_package": meta["package"],
        "urdf_path": f"urdf/{type_}.urdf.xacro",
        "xacro_args": "",
        "category": vendor,
        "attach": {"tool_frame": "tool0"},
        "specifications": {"degrees_of_freedom": scrape_dof(vendor, type_)},
        "required_packages": [meta["package"]],
        "tags": ["industrial"],
    }
