#include <gtest/gtest.h>
#include <cstdlib>
#include "robot_catalog_core/catalog.hpp"
#include "robot_catalog_core/json.hpp"

using robot_catalog::RobotCatalogCore;
using robot_catalog::RobotFilter;
using robot_catalog::RobotConfig;

static std::string yaml_path() {
  const char* p = std::getenv("RDSA_ROBOTS_YAML");
  return p ? p : "";
}

class CatalogTest : public ::testing::Test {
protected:
  RobotCatalogCore cat;
  void SetUp() override { cat.load(yaml_path()); }
};

TEST_F(CatalogTest, LoadsAllRobotsAndCategories) {
  EXPECT_EQ(cat.getAllRobots().size(), 27u);  // 20 arms + 2 EE + 5 bases
  EXPECT_EQ(cat.getCategories().size(), 5u);
}

TEST_F(CatalogTest, Ur3Fields) {
  RobotConfig r;
  ASSERT_TRUE(cat.getRobotById("ur3", r));
  EXPECT_EQ(r.display_name, "UR3");
  EXPECT_EQ(r.urdf_package, "ur_description");
  EXPECT_EQ(r.category, "universal_robots");
  EXPECT_EQ(r.specifications.degrees_of_freedom, 6);
  EXPECT_DOUBLE_EQ(r.specifications.payload_kg, 3.0);
  EXPECT_TRUE(r.specifications.collaborative);
}

TEST_F(CatalogTest, FilterByCategory) {
  RobotFilter f;
  f.category = "universal_robots";
  EXPECT_EQ(cat.filterRobots(f).size(), 9u);
}

TEST_F(CatalogTest, EmptyFilterReturnsAll) {
  EXPECT_EQ(cat.filterRobots(RobotFilter{}).size(), 27u);
}

TEST_F(CatalogTest, SearchCaseInsensitive) {
  EXPECT_EQ(cat.searchRobots("ur3").size(), cat.searchRobots("UR3").size());
  EXPECT_GT(cat.searchRobots("ur3").size(), 0u);
}

TEST_F(CatalogTest, JsonRoundtripShape) {
  auto j = robot_catalog::robots_to_json(cat.getAllRobots());
  EXPECT_NE(j.find("\"id\":\"ur3\""), std::string::npos);
  EXPECT_EQ(j.front(), '[');
}
