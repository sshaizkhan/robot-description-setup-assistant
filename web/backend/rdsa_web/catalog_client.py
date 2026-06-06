"""Bridge: call the C++ robot_catalog_server over ROS services via rclpy."""
from __future__ import annotations

import json
import threading

from .models import CategoryInfo, RobotConfig, RobotFilter


def filter_to_request_fields(f: RobotFilter) -> dict:
    return {
        "type": f.type or "",
        "category": f.category or "",
        "has_min_payload": f.min_payload is not None,
        "min_payload": f.min_payload or 0.0,
        "has_max_payload": f.max_payload is not None,
        "max_payload": f.max_payload or 0.0,
        "has_min_reach": f.min_reach is not None,
        "min_reach": f.min_reach or 0.0,
        "has_max_reach": f.max_reach is not None,
        "max_reach": f.max_reach or 0.0,
        "has_dof": f.degrees_of_freedom is not None,
        "degrees_of_freedom": f.degrees_of_freedom or 0,
        "has_collaborative_only": f.collaborative_only is not None,
        "collaborative_only": bool(f.collaborative_only),
        "required_tags": list(f.required_tags),
        "search_text": f.search_text or "",
    }


def parse_robots_json(text: str) -> list[RobotConfig]:
    return [RobotConfig(**r) for r in json.loads(text)]


def parse_categories_json(text: str) -> list[CategoryInfo]:
    return [CategoryInfo(**c) for c in json.loads(text)]


class CatalogServiceUnavailable(Exception):
    """Raised when the C++ catalog node is unreachable or times out."""


# Hard cap on mesh/image size, mirrored from the C++ side (kMeshSizeCap).
MESH_SIZE_CAP = 5 * 1024 * 1024  # 5 MiB


class MeshTooLarge(Exception):
    """Raised when a mesh/image exceeds the size cap; the asset must be shrunk."""

    def __init__(self, size: int, cap: int = MESH_SIZE_CAP) -> None:
        self.size = size
        self.cap = cap
        super().__init__(
            f"mesh too large: {size} bytes exceeds the {cap} byte cap"
        )


class CatalogClient:
    """rclpy client over the C++ robot_catalog_server.

    A background executor spins the node continuously so many service calls can
    be in flight at once (the FastAPI thread pool issues them concurrently —
    e.g. all of a robot's mesh requests). Resolved mesh bytes are cached so a
    re-open never re-crosses ROS.
    """

    def __init__(self, timeout_sec: float = 5.0) -> None:
        import rclpy
        from rclpy.executors import SingleThreadedExecutor
        from robot_catalog_msgs.srv import (
            FilterRobots,
            GetCategories,
            GetRobots,
            GetUrdf,
            ResolveMesh,
            ValidateRobot,
        )

        if not rclpy.ok():
            rclpy.init()
        self._rclpy = rclpy
        self._node = rclpy.create_node("rdsa_catalog_client")
        self._timeout = timeout_sec
        self._types = {
            "GetRobots": GetRobots,
            "GetCategories": GetCategories,
            "FilterRobots": FilterRobots,
            "ValidateRobot": ValidateRobot,
            "GetUrdf": GetUrdf,
            "ResolveMesh": ResolveMesh,
        }
        self._get_robots = self._node.create_client(GetRobots, "catalog/get_robots")
        self._get_categories = self._node.create_client(
            GetCategories, "catalog/get_categories"
        )
        self._filter = self._node.create_client(FilterRobots, "catalog/filter_robots")
        self._validate = self._node.create_client(ValidateRobot, "catalog/validate_robot")
        self._get_urdf = self._node.create_client(GetUrdf, "catalog/get_urdf")
        self._resolve_mesh = self._node.create_client(ResolveMesh, "catalog/resolve_mesh")

        # Spin the node in a background thread so concurrent call_async() futures
        # all complete without each caller having to drive the executor.
        self._executor = SingleThreadedExecutor()
        self._executor.add_node(self._node)
        self._spin_thread = threading.Thread(target=self._executor.spin, daemon=True)
        self._spin_thread.start()

        # Resolved-mesh byte cache: (package, rel_path) -> (bytes, media_type).
        # Meshes are immutable for a given install, so entries never expire.
        self._mesh_cache: dict = {}
        self._mesh_cache_lock = threading.Lock()
        # URDF (xacro) output is deterministic per robot; cache it too.
        self._urdf_cache: dict = {}
        self._urdf_cache_lock = threading.Lock()

    def _call(self, client, request):
        # No global lock: concurrent calls are the whole point. The background
        # executor completes each future; we block this caller's thread on it.
        if not client.service_is_ready():
            if not client.wait_for_service(timeout_sec=self._timeout):
                raise CatalogServiceUnavailable("robot_catalog_server not available")
        future = client.call_async(request)
        done = threading.Event()
        future.add_done_callback(lambda _f: done.set())
        if not done.wait(self._timeout):
            raise CatalogServiceUnavailable("catalog service call timed out")
        result = future.result()
        if result is None:
            raise CatalogServiceUnavailable("catalog service call failed")
        return result

    def get_all_robots(self) -> list[RobotConfig]:
        return self.get_robots_page(0, 0)[0]

    def get_robots_page(
        self, offset: int = 0, limit: int = 0
    ) -> "tuple[list[RobotConfig], int]":
        req = self._types["GetRobots"].Request()
        req.offset = int(offset)
        req.limit = int(limit)
        res = self._call(self._get_robots, req)
        return parse_robots_json(res.robots_json), int(res.total)

    def get_categories(self) -> list[CategoryInfo]:
        res = self._call(self._get_categories, self._types["GetCategories"].Request())
        return parse_categories_json(res.categories_json)

    def filter_robots(self, flt: RobotFilter) -> list[RobotConfig]:
        return self.filter_robots_page(flt, 0, 0)[0]

    def filter_robots_page(
        self, flt: RobotFilter, offset: int = 0, limit: int = 0
    ) -> "tuple[list[RobotConfig], int]":
        req = self._types["FilterRobots"].Request()
        for key, value in filter_to_request_fields(flt).items():
            setattr(req, key, value)
        req.offset = int(offset)
        req.limit = int(limit)
        res = self._call(self._filter, req)
        return parse_robots_json(res.robots_json), int(res.total)

    def validate(self, robot_id: str) -> dict:
        req = self._types["ValidateRobot"].Request()
        req.robot_id = robot_id
        res = self._call(self._validate, req)
        return {
            "found": bool(res.found),
            "ok": bool(res.ok),
            "missing_packages": list(res.missing_packages),
        }

    def get_urdf(self, robot_id: str) -> dict:
        cached = self._urdf_cache.get(robot_id)
        if cached is not None:
            return cached
        req = self._types["GetUrdf"].Request()
        req.robot_id = robot_id
        res = self._call(self._get_urdf, req)
        result = {
            "found": bool(res.found),
            "ok": bool(res.ok),
            "urdf_xml": res.urdf_xml,
            "missing_packages": list(res.missing_packages),
            "error": res.error,
        }
        # Cache only a successfully-expanded URDF (errors may be transient).
        if result["found"] and result["ok"]:
            with self._urdf_cache_lock:
                self._urdf_cache[robot_id] = result
        return result

    def resolve_mesh(self, package: str, rel_path: str) -> "tuple[bytes, str] | None":
        key = (package, rel_path)
        cached = self._mesh_cache.get(key)
        if cached is not None:
            return cached
        req = self._types["ResolveMesh"].Request()
        req.package = package
        req.rel_path = rel_path
        res = self._call(self._resolve_mesh, req)
        if res.ok:
            value = (bytes(res.data), res.media_type)
            with self._mesh_cache_lock:
                self._mesh_cache[key] = value
            return value
        if res.too_large:
            raise MeshTooLarge(int(res.size_bytes))
        return None


class CppCatalog:
    """Adapter exposing the RobotCatalog interface, backed by the C++ node."""

    def __init__(self, client: "CatalogClient | None" = None) -> None:
        self._client = client or CatalogClient()
        # The C++ node loads the catalog once at startup and only reads it
        # thereafter, so a by-id index is safe to memoize for the process.
        self._by_id: dict[str, RobotConfig] | None = None

    def get_all_robots(self) -> list[RobotConfig]:
        return self._client.get_all_robots()

    def get_robots_page(
        self, offset: int = 0, limit: int = 0
    ) -> "tuple[list[RobotConfig], int]":
        return self._client.get_robots_page(offset, limit)

    def get_categories(self) -> list[CategoryInfo]:
        return self._client.get_categories()

    def filter_robots(self, flt: RobotFilter) -> list[RobotConfig]:
        return self._client.filter_robots(flt)

    def filter_robots_page(
        self, flt: RobotFilter, offset: int = 0, limit: int = 0
    ) -> "tuple[list[RobotConfig], int]":
        return self._client.filter_robots_page(flt, offset, limit)

    def get_robot_by_id(self, robot_id: str) -> RobotConfig | None:
        # Memoized by-id index: avoids a full GetRobots round-trip + parse on
        # every /image, /package, and /{id} request (a card grid fires one
        # /image per visible card).
        if self._by_id is None:
            self._by_id = {r.id: r for r in self._client.get_all_robots()}
        return self._by_id.get(robot_id)

    # -- heavy work delegated to C++; this layer only relays --------------
    def get_urdf(self, robot_id: str) -> dict:
        return self._client.get_urdf(robot_id)

    def resolve_mesh(self, package: str, rel_path: str) -> "tuple[bytes, str] | None":
        return self._client.resolve_mesh(package, rel_path)

    def validate(self, robot_id: str) -> dict:
        return self._client.validate(robot_id)
