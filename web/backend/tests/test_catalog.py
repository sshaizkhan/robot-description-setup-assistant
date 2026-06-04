from rdsa_web.catalog import RobotCatalog


def test_loads_all_robots_and_categories(robots_yaml_path):
    cat = RobotCatalog.from_file(robots_yaml_path)
    assert len(cat.get_all_robots()) == 20
    assert len(cat.get_categories()) == 3


def test_ur3_fields_parsed(robots_yaml_path):
    cat = RobotCatalog.from_file(robots_yaml_path)
    ur3 = cat.get_robot_by_id("ur3")
    assert ur3 is not None
    assert ur3.display_name == "UR3"
    assert ur3.urdf_package == "ur_description"
    assert ur3.urdf_path == "urdf/ur.urdf.xacro"
    assert ur3.xacro_args == "name:=ur3_robot ur_type:=ur3"
    assert ur3.category == "universal_robots"
    assert ur3.specifications.degrees_of_freedom == 6
    assert ur3.specifications.payload_kg == 3.0
    assert ur3.specifications.reach_mm == 500
    assert ur3.specifications.collaborative is True
    assert "collaborative" in ur3.tags
    assert ur3.required_packages == ["ur_description", "ur_msgs"]


def test_mounting_key_maps_to_mounting_options(robots_yaml_path):
    cat = RobotCatalog.from_file(robots_yaml_path)
    ur3 = cat.get_robot_by_id("ur3")
    assert "floor" in ur3.specifications.mounting_options


def test_unknown_robot_id_returns_none(robots_yaml_path):
    cat = RobotCatalog.from_file(robots_yaml_path)
    assert cat.get_robot_by_id("does_not_exist") is None
