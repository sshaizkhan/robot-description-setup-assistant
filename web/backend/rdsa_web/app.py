"""FastAPI application factory and routes."""
from __future__ import annotations

import asyncio
import os
from pathlib import Path

from fastapi import FastAPI, HTTPException, Response, WebSocket, WebSocketDisconnect
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles

from .catalog import RobotCatalog
from .meshes import resolve_mesh_path
from .models import CategoryInfo, RobotConfig, RobotFilter
from .package_gen import build_package_zip, package_name
from .ros_bridge import JointStateRelay
from .urdf import UrdfError, resolve_urdf
from .validation import get_missing_packages


def _default_catalog() -> RobotCatalog:
    """Locate robots.yaml: env override first, then the installed package."""
    override = os.environ.get("RDSA_ROBOTS_YAML")
    if override:
        return RobotCatalog.from_file(Path(override))
    from ament_index_python.packages import get_package_share_directory

    share = Path(get_package_share_directory("robot_description_setup_assistant"))
    return RobotCatalog.from_file(share / "config" / "robots.yaml")


def create_app(catalog: RobotCatalog | None = None, relay=None, frontend_dist=None) -> FastAPI:
    app = FastAPI(title="RDSA Web", version="0.1.0")
    catalog = catalog or _default_catalog()
    relay = relay if relay is not None else JointStateRelay()
    frontend_dist = frontend_dist or os.environ.get("RDSA_FRONTEND_DIST")

    @app.get("/api/health")
    def health() -> dict[str, str]:
        return {"status": "ok"}

    @app.get("/api/categories", response_model=list[CategoryInfo])
    def categories() -> list[CategoryInfo]:
        return catalog.get_categories()

    @app.get("/api/robots", response_model=list[RobotConfig])
    def robots() -> list[RobotConfig]:
        return catalog.get_all_robots()

    @app.post("/api/robots/filter", response_model=list[RobotConfig])
    def filter_robots(flt: RobotFilter) -> list[RobotConfig]:
        return catalog.filter_robots(flt)

    @app.get("/api/robots/{robot_id}", response_model=RobotConfig)
    def robot(robot_id: str) -> RobotConfig:
        found = catalog.get_robot_by_id(robot_id)
        if found is None:
            raise HTTPException(status_code=404, detail=f"unknown robot: {robot_id}")
        return found

    @app.get("/api/robots/{robot_id}/validate")
    def validate(robot_id: str) -> dict:
        found = catalog.get_robot_by_id(robot_id)
        if found is None:
            raise HTTPException(status_code=404, detail=f"unknown robot: {robot_id}")
        missing = get_missing_packages(found)
        return {"ok": not missing, "missing_packages": missing}

    @app.get("/api/robots/{robot_id}/urdf")
    def robot_urdf(robot_id: str) -> dict:
        found = catalog.get_robot_by_id(robot_id)
        if found is None:
            raise HTTPException(status_code=404, detail=f"unknown robot: {robot_id}")
        missing = get_missing_packages(found)
        xml = ""
        err = ""
        try:
            xml = resolve_urdf(found)
        except UrdfError as exc:
            err = str(exc)
        return {
            "urdf_xml": xml,
            "mesh_base": "/meshes",
            "missing_packages": missing,
            "error": err,
        }

    @app.get("/meshes/{pkg}/{rel:path}")
    def mesh(pkg: str, rel: str) -> FileResponse:
        path = resolve_mesh_path(pkg, rel)
        if path is None:
            raise HTTPException(status_code=404, detail="mesh not found")
        return FileResponse(path)

    @app.get("/api/robots/{robot_id}/image")
    def robot_image(robot_id: str) -> FileResponse:
        found = catalog.get_robot_by_id(robot_id)
        if found is None:
            raise HTTPException(status_code=404, detail=f"unknown robot: {robot_id}")
        path = (
            resolve_mesh_path("robot_description_setup_assistant", found.image_path)
            if found.image_path
            else None
        )
        if path is None:
            raise HTTPException(status_code=404, detail="image not found")
        return FileResponse(path)

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

    if frontend_dist and Path(frontend_dist).is_dir():
        app.mount("/", StaticFiles(directory=frontend_dist, html=True), name="spa")

    return app


app = create_app()
