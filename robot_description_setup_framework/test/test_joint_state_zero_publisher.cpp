#include <gtest/gtest.h>
#include <chrono>
#include <memory>
#include <thread>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include "robot_description_setup_framework/qt/joint_state_zero_publisher.hpp"

using robot_description::setup_framework::JointStateZeroPublisher;
using namespace std::chrono_literals;

TEST(JointStateZeroPublisher, PublishesZerosForSetJoints)
{
  auto pub_node = std::make_shared<JointStateZeroPublisher>();
  pub_node->setJoints({ "a", "b", "c" });

  auto sub_node = std::make_shared<rclcpp::Node>("jsp_test_sub");
  sensor_msgs::msg::JointState last;
  bool got = false;
  auto sub = sub_node->create_subscription<sensor_msgs::msg::JointState>(
      "joint_states", 10, [&](sensor_msgs::msg::JointState::SharedPtr msg) {
        last = *msg;
        got = true;
      });

  rclcpp::executors::SingleThreadedExecutor exec;
  exec.add_node(pub_node);
  exec.add_node(sub_node);

  auto deadline = std::chrono::steady_clock::now() + 2s;
  while (!got && std::chrono::steady_clock::now() < deadline)
  {
    exec.spin_some();
    std::this_thread::sleep_for(10ms);
  }

  ASSERT_TRUE(got);
  ASSERT_EQ(last.name.size(), 3u);
  ASSERT_EQ(last.position.size(), 3u);
  EXPECT_EQ(last.name[0], "a");
  EXPECT_DOUBLE_EQ(last.position[0], 0.0);
  EXPECT_DOUBLE_EQ(last.position[2], 0.0);
}

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  ::testing::InitGoogleTest(&argc, argv);
  int rc = RUN_ALL_TESTS();
  rclcpp::shutdown();
  return rc;
}
