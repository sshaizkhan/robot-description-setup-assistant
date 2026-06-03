#include <gtest/gtest.h>
#include "robot_description_setup_framework/process_utils.hpp"

using robot_description::runProcess;

TEST(RunProcess, CapturesStdoutAndZeroExit)
{
  std::string out, err;
  int code = runProcess({ "echo", "hello" }, out, err);
  EXPECT_EQ(code, 0);
  EXPECT_EQ(out, "hello\n");
  EXPECT_TRUE(err.empty());
}

TEST(RunProcess, NonZeroExitCode)
{
  std::string out, err;
  int code = runProcess({ "false" }, out, err);
  EXPECT_NE(code, 0);
}

TEST(RunProcess, MissingBinaryReturnsNegative)
{
  std::string out, err;
  int code = runProcess({ "definitely_not_a_real_binary_xyz" }, out, err);
  EXPECT_EQ(code, -1);
}

TEST(RunProcess, CapturesStderr)
{
  std::string out, err;
  int code = runProcess({ "sh", "-c", "echo oops 1>&2" }, out, err);
  EXPECT_EQ(code, 0);
  EXPECT_EQ(err, "oops\n");
}
