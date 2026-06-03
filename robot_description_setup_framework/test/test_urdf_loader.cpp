#include <gtest/gtest.h>
#include <filesystem>
#include <stdexcept>
#include "robot_description_setup_framework/urdf_loader.hpp"

using robot_description::URDFLoader;
using robot_description::URDFModel;

// FIXTURE_DIR is defined via target_compile_definitions in CMake.
static std::filesystem::path fixture(const std::string& name)
{
  return std::filesystem::path(FIXTURE_DIR) / name;
}

TEST(URDFLoader, LoadsXacroAndEnumeratesMovableJoints)
{
  URDFModel m = URDFLoader::load(fixture("simple_arm.urdf.xacro"), "name:=simple_arm");

  EXPECT_FALSE(m.xml.empty());
  EXPECT_EQ(m.robot_name, "simple_arm");
  EXPECT_EQ(m.root_link, "base_link");
  ASSERT_EQ(m.movable_joints.size(), 1u);  // fixed_joint excluded
  EXPECT_EQ(m.movable_joints.front(), "joint1");
}

TEST(URDFLoader, MissingFileThrows)
{
  EXPECT_THROW(URDFLoader::load(fixture("nope.urdf.xacro"), ""), std::runtime_error);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
