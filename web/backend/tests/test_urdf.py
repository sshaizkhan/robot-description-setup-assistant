import shutil

import pytest

from rdsa_web import urdf
from rdsa_web.models import RobotConfig


def test_xacro_args_are_tokenized_with_shlex():
    # Quoted value with spaces must remain a single token.
    tokens = urdf.tokenize_xacro_args('name:=ur3 desc:="a b c"')
    assert tokens == ["name:=ur3", "desc:=a b c"]


def test_resolve_raises_for_unknown_package():
    robot = RobotConfig(
        id="x", display_name="X", urdf_package="definitely_missing_pkg",
        urdf_path="urdf/x.xacro",
    )
    with pytest.raises(urdf.UrdfError):
        urdf.resolve_urdf(robot)


@pytest.mark.skipif(shutil.which("xacro") is None, reason="xacro CLI not on PATH")
def test_resolve_ur3_produces_robot_xml(robots_yaml_path):
    from rdsa_web.catalog import RobotCatalog

    try:
        from ament_index_python.packages import get_package_share_directory
        get_package_share_directory("ur_description")
    except Exception:
        pytest.skip("ur_description not installed")

    ur3 = RobotCatalog.from_file(robots_yaml_path).get_robot_by_id("ur3")
    xml = urdf.resolve_urdf(ur3)
    assert "<robot" in xml
    assert "package://ur_description/" in xml
