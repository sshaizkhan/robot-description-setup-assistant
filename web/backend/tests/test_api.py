from pathlib import Path

from fastapi.testclient import TestClient

from rdsa_web.app import create_app
from rdsa_web.catalog import RobotCatalog

# web/backend/tests/test_api.py -> repo root is 3 levels up.
ROBOTS_YAML = (
    Path(__file__).resolve().parents[3]
    / "robot_description_setup_assistant"
    / "config"
    / "robots.yaml"
)


def _client():
    app = create_app(catalog=RobotCatalog.from_file(ROBOTS_YAML))
    return TestClient(app)


def test_health_returns_ok():
    client = TestClient(create_app(catalog=RobotCatalog.from_file(ROBOTS_YAML)))
    resp = client.get("/api/health")
    assert resp.status_code == 200
    assert resp.json() == {"status": "ok"}


def test_get_categories():
    resp = _client().get("/api/categories")
    assert resp.status_code == 200
    assert len(resp.json()) == 3


def test_get_robots():
    resp = _client().get("/api/robots")
    assert resp.status_code == 200
    assert len(resp.json()) == 20


def test_get_robot_by_id():
    resp = _client().get("/api/robots/ur3")
    assert resp.status_code == 200
    assert resp.json()["display_name"] == "UR3"


def test_get_robot_by_id_404():
    resp = _client().get("/api/robots/nope")
    assert resp.status_code == 404


def test_filter_robots_endpoint():
    resp = _client().post("/api/robots/filter", json={"category": "universal_robots"})
    assert resp.status_code == 200
    assert len(resp.json()) == 9


def test_validate_endpoint_shape():
    resp = _client().get("/api/robots/ur3/validate")
    assert resp.status_code == 200
    body = resp.json()
    assert set(body.keys()) == {"ok", "missing_packages"}
    assert isinstance(body["missing_packages"], list)


def test_urdf_endpoint_shape():
    resp = _client().get("/api/robots/ur3/urdf")
    assert resp.status_code == 200
    body = resp.json()
    assert set(body.keys()) == {"urdf_xml", "mesh_base", "missing_packages", "error"}
    assert body["mesh_base"] == "/meshes"
    assert isinstance(body["urdf_xml"], str)
    assert isinstance(body["missing_packages"], list)


def test_urdf_endpoint_404_for_unknown_robot():
    resp = _client().get("/api/robots/nope/urdf")
    assert resp.status_code == 404


def test_mesh_endpoint_404_for_unknown_package():
    resp = _client().get("/meshes/definitely_missing_pkg/meshes/x.stl")
    assert resp.status_code == 404


def test_image_endpoint_404_for_unknown_robot():
    resp = _client().get("/api/robots/nope/image")
    assert resp.status_code == 404


def test_image_endpoint_404_when_no_image_path(monkeypatch):
    # A robot whose image cannot be resolved yields 404, not a 500.
    import rdsa_web.app as appmod

    monkeypatch.setattr(appmod, "resolve_mesh_path", lambda pkg, rel: None)
    resp = _client().get("/api/robots/ur3/image")
    assert resp.status_code == 404


def test_ws_joint_states_streams_relay_snapshot():
    from rdsa_web.app import create_app
    from rdsa_web.catalog import RobotCatalog

    class FakeRelay:
        started = False

        def start(self):
            self.started = True

        def latest(self):
            return {"shoulder_pan_joint": 0.5}

    relay = FakeRelay()
    app = create_app(catalog=RobotCatalog.from_file(ROBOTS_YAML), relay=relay)
    client = TestClient(app)
    with client.websocket_connect("/ws/joint_states") as ws:
        msg = ws.receive_json()
    assert relay.started is True
    assert msg["joints"] == {"shoulder_pan_joint": 0.5}


def test_serves_spa_when_dist_exists(tmp_path):
    from rdsa_web.app import create_app
    from rdsa_web.catalog import RobotCatalog

    dist = tmp_path / "dist"
    dist.mkdir()
    (dist / "index.html").write_text("<!doctype html><title>spa</title>")

    app = create_app(
        catalog=RobotCatalog.from_file(ROBOTS_YAML), frontend_dist=str(dist)
    )
    client = TestClient(app)
    resp = client.get("/")
    assert resp.status_code == 200
    assert "spa" in resp.text


def test_api_still_wins_over_spa_mount(tmp_path):
    from rdsa_web.app import create_app
    from rdsa_web.catalog import RobotCatalog

    dist = tmp_path / "dist"
    dist.mkdir()
    (dist / "index.html").write_text("spa")

    app = create_app(
        catalog=RobotCatalog.from_file(ROBOTS_YAML), frontend_dist=str(dist)
    )
    client = TestClient(app)
    assert len(client.get("/api/robots").json()) == 20


def test_no_spa_mount_without_dist():
    # The default app (no dist) returns 404 at / — API-only mode is intact.
    resp = _client().get("/")
    assert resp.status_code == 404


def test_package_endpoint_returns_zip():
    import io
    import zipfile

    resp = _client().get("/api/robots/ur3/package")
    assert resp.status_code == 200
    assert resp.headers["content-type"] == "application/zip"
    with zipfile.ZipFile(io.BytesIO(resp.content)) as zf:
        assert "ur3_bringup/launch/view.launch.py" in zf.namelist()


def test_package_endpoint_404_for_unknown_robot():
    resp = _client().get("/api/robots/nope/package")
    assert resp.status_code == 404
