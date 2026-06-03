#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>
#include "robot_description_setup_framework/app_context.hpp"

using robot_description::AppContext;
using robot_description::URDFModel;

TEST(AppContext, HoldsNodeAndFields)
{
  auto node = std::make_shared<rclcpp::Node>("test_app_context");
  AppContext ctx(node);

  EXPECT_EQ(ctx.getNode(), node);
  EXPECT_FALSE(ctx.debug);
  EXPECT_FALSE(ctx.current_urdf.has_value());

  URDFModel m;
  m.robot_name = "arm";
  m.movable_joints = { "j1", "j2" };
  m.root_link = "base";
  ctx.current_urdf = m;

  ASSERT_TRUE(ctx.current_urdf.has_value());
  EXPECT_EQ(ctx.current_urdf->robot_name, "arm");
  EXPECT_EQ(ctx.current_urdf->movable_joints.size(), 2u);
}

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  ::testing::InitGoogleTest(&argc, argv);
  int rc = RUN_ALL_TESTS();
  rclcpp::shutdown();
  return rc;
}
