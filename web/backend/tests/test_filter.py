from rdsa_web.catalog import RobotCatalog
from rdsa_web.models import RobotFilter


def _cat(p):
    return RobotCatalog.from_file(p)


def test_empty_filter_returns_all(robots_yaml_path):
    cat = _cat(robots_yaml_path)
    assert len(cat.filter_robots(RobotFilter())) == 27


def test_filter_by_type(robots_yaml_path):
    cat = _cat(robots_yaml_path)
    assert len(cat.filter_robots(RobotFilter(type="arm"))) == 20
    assert len(cat.filter_robots(RobotFilter(type="end_effector"))) == 2
    assert len(cat.filter_robots(RobotFilter(type="base"))) == 5


def test_filter_by_category(robots_yaml_path):
    cat = _cat(robots_yaml_path)
    result = cat.filter_robots(RobotFilter(category="universal_robots"))
    assert len(result) == 9
    assert all(r.category == "universal_robots" for r in result)


def test_filter_collaborative_only(robots_yaml_path):
    cat = _cat(robots_yaml_path)
    result = cat.filter_robots(RobotFilter(collaborative_only=True))
    assert all(r.specifications.collaborative for r in result)
    assert any(r.id == "ur3" for r in result)


def test_filter_min_payload(robots_yaml_path):
    cat = _cat(robots_yaml_path)
    result = cat.filter_robots(RobotFilter(min_payload=10.0))
    assert all(r.specifications.payload_kg >= 10.0 for r in result)


def test_filter_required_tags_all_must_match(robots_yaml_path):
    cat = _cat(robots_yaml_path)
    result = cat.filter_robots(RobotFilter(required_tags=["collaborative", "compact"]))
    assert all(
        "collaborative" in r.tags and "compact" in r.tags for r in result
    )
    assert any(r.id == "ur3" for r in result)
