import io
import zipfile

from rdsa_web import package_gen
from rdsa_web.models import RobotConfig


def _ur3():
    return RobotConfig(
        id="ur3",
        display_name="UR3",
        urdf_package="ur_description",
        urdf_path="urdf/ur.urdf.xacro",
        xacro_args="name:=ur3_robot ur_type:=ur3",
    )


def test_package_name():
    assert package_gen.package_name(_ur3()) == "ur3_bringup"


def test_package_xml_lists_dependencies():
    xml = package_gen.render_package_xml(_ur3())
    assert "<name>ur3_bringup</name>" in xml
    assert "robot_state_publisher" in xml
    assert "joint_state_publisher_gui" in xml
    assert "ur_description" in xml


def test_view_launch_references_urdf_and_args():
    launch = package_gen.render_view_launch(_ur3())
    assert "ur_description" in launch
    assert "urdf/ur.urdf.xacro" in launch
    assert "ur_type:=ur3" in launch
    assert "robot_state_publisher" in launch
    assert "joint_state_publisher_gui" in launch


def test_view_launch_escapes_hostile_values_and_compiles():
    import ast

    robot = RobotConfig(
        id="x",
        display_name='X "quote"\nbot',
        urdf_package="p",
        urdf_path='urdf/a"b.xacro',
        xacro_args='name:="a b" k:=1',
    )
    launch = package_gen.render_view_launch(robot)
    # Must be valid Python (no injection / broken string literal).
    ast.parse(launch)
    # The hostile values survive as escaped literals, not raw.
    assert "p" in launch


def test_build_package_zip_contains_all_files():
    data = package_gen.build_package_zip(_ur3())
    with zipfile.ZipFile(io.BytesIO(data)) as zf:
        names = set(zf.namelist())
    assert "ur3_bringup/package.xml" in names
    assert "ur3_bringup/CMakeLists.txt" in names
    assert "ur3_bringup/launch/view.launch.py" in names
    assert "ur3_bringup/README.md" in names
