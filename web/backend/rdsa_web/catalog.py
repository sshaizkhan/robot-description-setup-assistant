"""Robot catalog: loads robots.yaml into models and provides queries."""
from __future__ import annotations

from pathlib import Path

import yaml

from .models import (
    AttachInfo,
    CategoryInfo,
    RobotConfig,
    RobotFilter,
    RobotSpecifications,
)


class RobotCatalog:
    def __init__(
        self,
        robots: dict[str, RobotConfig],
        categories: dict[str, CategoryInfo],
    ) -> None:
        self._robots = robots
        self._categories = categories

    # -- construction --------------------------------------------------
    @classmethod
    def from_file(cls, path: str | Path) -> "RobotCatalog":
        """Load a single YAML file, or every *.yml/*.yaml under a directory."""
        p = Path(path)
        if p.is_dir():
            return cls.from_directory(p)
        return cls._from_data([yaml.safe_load(p.read_text()) or {}])

    @classmethod
    def from_directory(cls, path: str | Path) -> "RobotCatalog":
        p = Path(path)
        files = sorted(set(p.rglob("*.yml")) | set(p.rglob("*.yaml")))
        return cls._from_data([yaml.safe_load(f.read_text()) or {} for f in files])

    @classmethod
    def _from_data(cls, datas: list[dict]) -> "RobotCatalog":
        categories: dict[str, CategoryInfo] = {}
        robots: dict[str, RobotConfig] = {}
        for data in datas:
            for cid, node in (data.get("categories") or {}).items():
                categories[cid] = cls._parse_category(cid, node)
            for rid, node in (data.get("robots") or {}).items():
                robots[rid] = cls._parse_robot(rid, node)
        return cls(robots, categories)

    @staticmethod
    def _parse_category(cid: str, node: dict) -> CategoryInfo:
        return CategoryInfo(
            id=cid,
            display_name=node.get("display_name", ""),
            description=node.get("description", ""),
            manufacturer=node.get("manufacturer", ""),
            website=node.get("website", ""),
        )

    @staticmethod
    def _parse_specs(node: dict | None) -> RobotSpecifications:
        node = node or {}
        return RobotSpecifications(
            degrees_of_freedom=node.get("degrees_of_freedom", 0),
            payload_kg=node.get("payload_kg", 0.0),
            reach_mm=node.get("reach_mm", 0.0),
            weight_kg=node.get("weight_kg", 0.0),
            repeatability_mm=node.get("repeatability_mm", 0.0),
            max_speed_ms=node.get("max_speed_ms", 0.0),
            mounting_options=list(node.get("mounting", []) or []),
            safety_certified=node.get("safety_certified", False),
            collaborative=node.get("collaborative", False),
            torque_sensing=node.get("torque_sensing", False),
        )

    @staticmethod
    def _parse_attach(node: dict | None) -> AttachInfo:
        node = node or {}
        return AttachInfo(
            tool_frame=node.get("tool_frame", ""),
            mount_frame=node.get("mount_frame", ""),
            xyz=list(node.get("xyz", [0.0, 0.0, 0.0]) or [0.0, 0.0, 0.0]),
            rpy=list(node.get("rpy", [0.0, 0.0, 0.0]) or [0.0, 0.0, 0.0]),
        )

    @classmethod
    def _parse_robot(cls, rid: str, node: dict) -> RobotConfig:
        return RobotConfig(
            id=rid,
            display_name=node.get("display_name", ""),
            description=node.get("description", ""),
            image_path=node.get("image_path", ""),
            urdf_package=node.get("urdf_package", ""),
            urdf_path=node.get("urdf_path", ""),
            xacro_args=node.get("xacro_args", ""),
            category=node.get("category", ""),
            type=node.get("type", "arm"),
            attach=cls._parse_attach(node.get("attach")),
            specifications=cls._parse_specs(node.get("specifications")),
            required_packages=list(node.get("required_packages", []) or []),
            optional_packages=list(node.get("optional_packages", []) or []),
            tags=list(node.get("tags", []) or []),
        )

    # -- queries -------------------------------------------------------
    def get_all_robots(self) -> list[RobotConfig]:
        return sorted(self._robots.values(), key=lambda r: r.id)

    def get_robots_page(
        self, offset: int = 0, limit: int = 0
    ) -> "tuple[list[RobotConfig], int]":
        """Stable page of all robots; limit <= 0 means all. Returns (page, total)."""
        robots = self.get_all_robots()
        total = len(robots)
        offset = max(offset, 0)
        end = total if limit <= 0 else min(total, offset + limit)
        return robots[offset:end], total

    def get_robot_by_id(self, robot_id: str) -> RobotConfig | None:
        return self._robots.get(robot_id)

    def get_categories(self) -> list[CategoryInfo]:
        return list(self._categories.values())

    def get_category_info(self, category_id: str) -> CategoryInfo | None:
        return self._categories.get(category_id)

    def get_robots_by_category(self, category: str) -> list[RobotConfig]:
        return [r for r in self._robots.values() if r.category == category]

    def filter_robots(self, flt: RobotFilter) -> list[RobotConfig]:
        if flt.is_empty():
            return self.get_all_robots()
        return [r for r in self.get_all_robots() if self._matches_filter(r, flt)]

    def filter_robots_page(
        self, flt: RobotFilter, offset: int = 0, limit: int = 0
    ) -> "tuple[list[RobotConfig], int]":
        matched = self.filter_robots(flt)
        total = len(matched)
        offset = max(offset, 0)
        end = total if limit <= 0 else min(total, offset + limit)
        return matched[offset:end], total

    @staticmethod
    def _matches_filter(r: RobotConfig, flt: RobotFilter) -> bool:
        s = r.specifications
        if flt.type is not None and r.type != flt.type:
            return False
        if flt.category is not None and r.category != flt.category:
            return False
        if flt.min_payload is not None and s.payload_kg < flt.min_payload:
            return False
        if flt.max_payload is not None and s.payload_kg > flt.max_payload:
            return False
        if flt.min_reach is not None and s.reach_mm < flt.min_reach:
            return False
        if flt.max_reach is not None and s.reach_mm > flt.max_reach:
            return False
        if (
            flt.degrees_of_freedom is not None
            and s.degrees_of_freedom != flt.degrees_of_freedom
        ):
            return False
        if flt.collaborative_only is True and not s.collaborative:
            return False
        if any(tag not in r.tags for tag in flt.required_tags):
            return False
        if flt.search_text and not RobotCatalog._matches_search(r, flt.search_text):
            return False
        return True

    def search_robots(self, term: str) -> list[RobotConfig]:
        if not term:
            return self.get_all_robots()
        return [r for r in self._robots.values() if self._matches_search(r, term)]

    @staticmethod
    def _matches_search(r: RobotConfig, term: str) -> bool:
        needle = term.lower()
        haystacks = [r.display_name, r.description, r.category, *r.tags]
        return any(needle in h.lower() for h in haystacks)

    # -- URDF / mesh / validation ----------------------------------------
    # Pure-Python fallback used only in no-ROS/test mode. In production the
    # C++ catalog backend (CppCatalog) performs all of this work; this class
    # mirrors that interface so app.py can stay backend-agnostic.
    def get_urdf(self, robot_id: str) -> dict:
        from .urdf import UrdfError, resolve_urdf
        from .validation import get_missing_packages

        robot = self.get_robot_by_id(robot_id)
        if robot is None:
            return {"found": False, "ok": False, "urdf_xml": "",
                    "missing_packages": [], "error": ""}
        missing = get_missing_packages(robot)
        try:
            xml = resolve_urdf(robot)
            return {"found": True, "ok": True, "urdf_xml": xml,
                    "missing_packages": missing, "error": ""}
        except UrdfError as exc:
            return {"found": True, "ok": False, "urdf_xml": "",
                    "missing_packages": missing, "error": str(exc)}

    def resolve_mesh(self, package: str, rel_path: str) -> "tuple[bytes, str] | None":
        import mimetypes

        from .catalog_client import MESH_SIZE_CAP, MeshTooLarge
        from .meshes import resolve_mesh_path

        path = resolve_mesh_path(package, rel_path)
        if path is None:
            return None
        size = path.stat().st_size
        if size > MESH_SIZE_CAP:
            raise MeshTooLarge(size)
        media_type = mimetypes.guess_type(str(path))[0] or "application/octet-stream"
        return path.read_bytes(), media_type

    def validate(self, robot_id: str) -> dict:
        from .validation import get_missing_packages

        robot = self.get_robot_by_id(robot_id)
        if robot is None:
            return {"found": False, "ok": False, "missing_packages": []}
        missing = get_missing_packages(robot)
        return {"found": True, "ok": not missing, "missing_packages": missing}
