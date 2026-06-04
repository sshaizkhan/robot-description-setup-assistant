from rdsa_web.catalog_client import filter_to_request_fields, parse_robots_json
from rdsa_web.models import RobotFilter


def test_filter_to_request_fields_sets_has_flags():
    f = RobotFilter(category="kuka", min_payload=5.0, collaborative_only=True)
    fields = filter_to_request_fields(f)
    assert fields["category"] == "kuka"
    assert fields["has_min_payload"] is True
    assert fields["min_payload"] == 5.0
    assert fields["has_collaborative_only"] is True
    assert fields["has_max_payload"] is False


def test_parse_robots_json_returns_models():
    robots = parse_robots_json(
        '[{"id":"ur3","display_name":"UR3","category":"universal_robots",'
        '"urdf_package":"ur_description"}]'
    )
    assert robots[0].id == "ur3"
    assert robots[0].display_name == "UR3"
    assert robots[0].urdf_package == "ur_description"
