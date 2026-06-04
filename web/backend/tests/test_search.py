from rdsa_web.catalog import RobotCatalog
from rdsa_web.models import RobotFilter


def _cat(p):
    return RobotCatalog.from_file(p)


def test_search_matches_display_name(robots_yaml_path):
    cat = _cat(robots_yaml_path)
    result = cat.search_robots("UR3")
    assert any(r.id == "ur3" for r in result)


def test_search_is_case_insensitive(robots_yaml_path):
    cat = _cat(robots_yaml_path)
    assert cat.search_robots("ur3") == cat.search_robots("UR3")


def test_search_matches_tag(robots_yaml_path):
    cat = _cat(robots_yaml_path)
    result = cat.search_robots("collaborative")
    assert any(r.id == "ur3" for r in result)


def test_search_via_filter_search_text(robots_yaml_path):
    cat = _cat(robots_yaml_path)
    result = cat.filter_robots(RobotFilter(search_text="UR3"))
    assert any(r.id == "ur3" for r in result)


def test_search_no_match_returns_empty(robots_yaml_path):
    cat = _cat(robots_yaml_path)
    assert cat.search_robots("zzzznotarobot") == []
