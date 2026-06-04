import rdsa_web.meshes as meshes


def _fake_pkg(tmp_path, monkeypatch):
    """Create a fake package share dir with one mesh file."""
    share = tmp_path / "share" / "fake_pkg"
    (share / "meshes").mkdir(parents=True)
    mesh = share / "meshes" / "link.stl"
    mesh.write_text("solid\nendsolid\n")
    secret = tmp_path / "secret.txt"
    secret.write_text("top secret")

    def fake_share(name):
        if name == "fake_pkg":
            return str(share)
        from ament_index_python.packages import PackageNotFoundError
        raise PackageNotFoundError(name)

    monkeypatch.setattr(meshes, "get_package_share_directory", fake_share)
    return share, mesh, secret


def test_resolves_valid_mesh(tmp_path, monkeypatch):
    _share, mesh, _secret = _fake_pkg(tmp_path, monkeypatch)
    result = meshes.resolve_mesh_path("fake_pkg", "meshes/link.stl")
    assert result == mesh.resolve()


def test_unknown_package_returns_none(tmp_path, monkeypatch):
    _fake_pkg(tmp_path, monkeypatch)
    assert meshes.resolve_mesh_path("nope_pkg", "meshes/link.stl") is None


def test_missing_file_returns_none(tmp_path, monkeypatch):
    _fake_pkg(tmp_path, monkeypatch)
    assert meshes.resolve_mesh_path("fake_pkg", "meshes/absent.stl") is None


def test_path_traversal_is_rejected(tmp_path, monkeypatch):
    _share, _mesh, _secret = _fake_pkg(tmp_path, monkeypatch)
    # Attempt to escape the package share dir to read secret.txt.
    assert meshes.resolve_mesh_path("fake_pkg", "../../secret.txt") is None
