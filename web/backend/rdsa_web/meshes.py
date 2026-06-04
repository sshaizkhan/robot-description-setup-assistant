"""Resolve package://-style mesh references to files on disk, safely."""
from __future__ import annotations

import os
from pathlib import Path

from ament_index_python.packages import (
    PackageNotFoundError,
    get_package_share_directory,
)


def _is_within(target: Path, base: Path) -> bool:
    try:
        target.relative_to(base)
        return True
    except ValueError:
        return False


def resolve_mesh_path(pkg: str, rel: str) -> Path | None:
    """Return the absolute file path for `pkg`/`rel`, or None if invalid.

    Rejects paths that escape the package share directory (traversal guard).

    Note: the package share dir is resolved canonically, but `..` in `rel` is
    collapsed lexically (os.path.normpath) WITHOUT resolving symlinks on the
    final file. Merged/symlink colcon installs symlink the mesh files to their
    source locations; resolving those symlinks would point outside the package
    share dir and wrongly fail the containment check.
    """
    try:
        base = Path(get_package_share_directory(pkg)).resolve()
    except PackageNotFoundError:
        return None

    target = Path(os.path.normpath(base / rel))
    if not _is_within(target, base):
        return None
    if not target.is_file():
        return None
    return target
