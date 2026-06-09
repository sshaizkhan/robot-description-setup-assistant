"""FastAPI application factory and routes."""
from __future__ import annotations

import asyncio
import logging
import os
from pathlib import Path

from fastapi import FastAPI, HTTPException, Response, WebSocket, WebSocketDisconnect
from fastapi.staticfiles import StaticFiles

from .catalog import RobotCatalog
from .catalog_client import CatalogServiceUnavailable, MeshTooLarge
from .models import CategoryInfo, RobotConfig, RobotFilter
from .package_gen import build_package_zip, package_name
from .ros_bridge import JointStateRelay, MarkerRelay, TfRelay


class _QuietPathsFilter(logging.Filter):
    """Drop uvicorn access-log lines for chatty static routes (one GET per mesh).

    Opening a robot fetches ~14 meshes; logging each just buries the API calls.
    Set RDSA_LOG_MESHES=1 to keep them.
    """

    def filter(self, record: logging.LogRecord) -> bool:
        args = record.args
        # uvicorn.access args: (client, method, path, http_version, status)
        if isinstance(args, tuple) and len(args) >= 3 and isinstance(args[2], str):
            path = args[2]
            if path.startswith("/meshes/") or path.endswith("/image"):
                return False
        return True


if os.environ.get("RDSA_LOG_MESHES") != "1":
    logging.getLogger("uvicorn.access").addFilter(_QuietPathsFilter())


def _default_catalog() -> RobotCatalog:
    """Locate robots.yaml: env override first, then the installed package."""
    override = os.environ.get("RDSA_ROBOTS_YAML")
    if override:
        return RobotCatalog.from_file(Path(override))
    from ament_index_python.packages import get_package_share_directory

    share = Path(get_package_share_directory("robot_description_setup_assistant"))
    # Prefer the multi-file catalog directory (arms + end-effectors + bases);
    # fall back to the legacy single file. Mirrors the C++ default_catalog().
    catalog_dir = share / "config" / "catalog"
    if catalog_dir.is_dir():
        return RobotCatalog.from_file(catalog_dir)
    return RobotCatalog.from_file(share / "config" / "robots.yaml")


def _make_catalog():
    """C++ service-backed catalog when RDSA_CATALOG_BACKEND=cpp, else YAML."""
    if os.environ.get("RDSA_CATALOG_BACKEND") == "cpp":
        from .catalog_client import CppCatalog

        return CppCatalog()
    return _default_catalog()


def create_app(
    catalog=None, relay=None, tf_relay=None, marker_relay=None, frontend_dist=None
) -> FastAPI:
    app = FastAPI(title="RDSA Web", version="0.1.0")
    catalog = catalog or _make_catalog()
    relay = relay if relay is not None else JointStateRelay()
    tf_relay = tf_relay if tf_relay is not None else TfRelay()
    marker_relay = marker_relay if marker_relay is not None else MarkerRelay()
    frontend_dist = frontend_dist or os.environ.get("RDSA_FRONTEND_DIST")

    @app.get("/api/health")
    def health() -> dict[str, str]:
        return {"status": "ok"}

    @app.get("/api/categories", response_model=list[CategoryInfo])
    def categories() -> list[CategoryInfo]:
        try:
            return catalog.get_categories()
        except CatalogServiceUnavailable as exc:
            raise HTTPException(status_code=503, detail=str(exc))

    @app.get("/api/robots", response_model=list[RobotConfig])
    def robots(
        response: Response,
        limit: int | None = None,
        offset: int = 0,
    ) -> list[RobotConfig]:
        # No `limit` → full list (unchanged default). With `limit`, return a
        # stable page and report the full count via the X-Total-Count header.
        try:
            if limit is None:
                items = catalog.get_all_robots()
                response.headers["X-Total-Count"] = str(len(items))
                return items
            page, total = catalog.get_robots_page(offset, limit)
            response.headers["X-Total-Count"] = str(total)
            return page
        except CatalogServiceUnavailable as exc:
            raise HTTPException(status_code=503, detail=str(exc))

    @app.post("/api/robots/filter", response_model=list[RobotConfig])
    def filter_robots(
        flt: RobotFilter,
        response: Response,
        limit: int | None = None,
        offset: int = 0,
    ) -> list[RobotConfig]:
        try:
            if limit is None:
                items = catalog.filter_robots(flt)
                response.headers["X-Total-Count"] = str(len(items))
                return items
            page, total = catalog.filter_robots_page(flt, offset, limit)
            response.headers["X-Total-Count"] = str(total)
            return page
        except CatalogServiceUnavailable as exc:
            raise HTTPException(status_code=503, detail=str(exc))

    @app.get("/api/robots/{robot_id}", response_model=RobotConfig)
    def robot(robot_id: str) -> RobotConfig:
        found = catalog.get_robot_by_id(robot_id)
        if found is None:
            raise HTTPException(status_code=404, detail=f"unknown robot: {robot_id}")
        return found

    @app.get("/api/robots/{robot_id}/validate")
    def validate(robot_id: str) -> dict:
        try:
            data = catalog.validate(robot_id)
        except CatalogServiceUnavailable as exc:
            raise HTTPException(status_code=503, detail=str(exc))
        if not data.get("found"):
            raise HTTPException(status_code=404, detail=f"unknown robot: {robot_id}")
        return {"ok": data["ok"], "missing_packages": data["missing_packages"]}

    @app.get("/api/robots/{robot_id}/urdf")
    def robot_urdf(robot_id: str) -> dict:
        # All xacro/package resolution happens in the C++ backend; we relay it.
        try:
            data = catalog.get_urdf(robot_id)
        except CatalogServiceUnavailable as exc:
            raise HTTPException(status_code=503, detail=str(exc))
        if not data.get("found"):
            raise HTTPException(status_code=404, detail=f"unknown robot: {robot_id}")
        return {
            "urdf_xml": data.get("urdf_xml", ""),
            "mesh_base": "/meshes",
            "missing_packages": data.get("missing_packages", []),
            "error": data.get("error", ""),
        }

    @app.get("/meshes/{pkg}/{rel:path}")
    def mesh(pkg: str, rel: str) -> Response:
        # C++ reads + resolves the file; Python relays the bytes (no filesystem).
        try:
            result = catalog.resolve_mesh(pkg, rel)
        except CatalogServiceUnavailable as exc:
            raise HTTPException(status_code=503, detail=str(exc))
        except MeshTooLarge as exc:
            raise HTTPException(status_code=413, detail=str(exc))
        if not result:
            raise HTTPException(status_code=404, detail="mesh not found")
        data, media_type = result
        return Response(
            content=data,
            media_type=media_type,
            headers={"Cache-Control": "public, max-age=86400"},
        )

    @app.get("/api/robots/{robot_id}/image")
    def robot_image(robot_id: str) -> Response:
        found = catalog.get_robot_by_id(robot_id)
        if found is None:
            raise HTTPException(status_code=404, detail=f"unknown robot: {robot_id}")
        try:
            result = (
                catalog.resolve_mesh(
                    "robot_description_setup_assistant", found.image_path
                )
                if found.image_path
                else None
            )
        except CatalogServiceUnavailable as exc:
            raise HTTPException(status_code=503, detail=str(exc))
        except MeshTooLarge as exc:
            raise HTTPException(status_code=413, detail=str(exc))
        if result is None:
            raise HTTPException(status_code=404, detail="image not found")
        data, media_type = result
        return Response(
            content=data,
            media_type=media_type,
            headers={"Cache-Control": "public, max-age=86400"},
        )

    @app.get("/api/robots/{robot_id}/package")
    def robot_package(robot_id: str) -> Response:
        found = catalog.get_robot_by_id(robot_id)
        if found is None:
            raise HTTPException(status_code=404, detail=f"unknown robot: {robot_id}")
        data = build_package_zip(found)
        filename = f"{package_name(found)}.zip"
        return Response(
            content=data,
            media_type="application/zip",
            headers={"Content-Disposition": f'attachment; filename="{filename}"'},
        )

    @app.websocket("/ws/joint_states")
    async def ws_joint_states(ws: WebSocket) -> None:
        await ws.accept()
        try:
            relay.start()
        except Exception as exc:  # ROS unavailable: keep socket alive, no joints
            await ws.send_json({"joints": {}, "error": str(exc)})
        try:
            while True:
                await ws.send_json({"joints": relay.latest()})
                await asyncio.sleep(0.1)
        except WebSocketDisconnect:
            return

    @app.websocket("/ws/tf")
    async def ws_tf(ws: WebSocket) -> None:
        await ws.accept()
        try:
            tf_relay.start()
        except Exception as exc:  # ROS unavailable: keep socket alive, no tf
            await ws.send_json({"transforms": [], "error": str(exc)})
        try:
            while True:
                await ws.send_json({"transforms": tf_relay.latest()})
                await asyncio.sleep(0.066)  # ~15 Hz cap (low-RAM box)
        except WebSocketDisconnect:
            return

    @app.websocket("/ws/markers")
    async def ws_markers(ws: WebSocket) -> None:
        await ws.accept()
        try:
            marker_relay.start()
        except Exception as exc:  # ROS unavailable: keep socket alive, no markers
            await ws.send_json({"markers": [], "error": str(exc)})
        try:
            while True:
                await ws.send_json({"markers": marker_relay.latest()})
                await asyncio.sleep(0.066)  # ~15 Hz cap
        except WebSocketDisconnect:
            return

    if frontend_dist and Path(frontend_dist).is_dir():
        app.mount("/", StaticFiles(directory=frontend_dist, html=True), name="spa")

    return app


app = create_app()
