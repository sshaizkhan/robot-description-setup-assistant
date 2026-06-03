# Remove MoveIt Dependency Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Remove all MoveIt dependencies from RDSA while keeping the working catalog → xacro → 3D-render flow with live robot switching.

**Architecture:** Replace MoveIt's setup framework (`DataWarehouse`/`URDFConfig`/`SRDFConfig`) and `moveit_rviz_plugin::RobotStateDisplay` with home-grown C++ units — `AppContext`, `URDFLoader`/`URDFModel`, `process_utils`, `JointStateZeroPublisher` — plus the stock `rviz_default_plugins/RobotModel` display fed by an embedded `robot_state_publisher` node. The `xacro` CLI is the only non-C++ piece (invoked as a subprocess).

**Tech Stack:** ROS 2 Humble, ament_cmake, C++17, Qt5, rclcpp, urdf (urdfdom), robot_state_publisher, sensor_msgs, rviz_common / rviz_default_plugins, GoogleTest (ament_cmake_gtest).

**Spec:** `docs/superpowers/specs/2026-06-02-remove-moveit-dependency-design.md`

**Branch:** `feat/remove-moveit-dependency`

---

## Build & Test Commands (reference)

All colcon commands run from the workspace root:

```bash
cd ~/rds_ws
# build one package
colcon build --packages-select robot_description_setup_framework --event-handlers console_direct+
# run one test and show results
colcon test --packages-select robot_description_setup_framework --ctest-args -R <test_name> --event-handlers console_direct+
colcon test-result --verbose
# source after build
source ~/rds_ws/install/setup.bash
```

> **Phase note:** Tasks 1–4 are **additive** — the framework still depends on MoveIt and builds green throughout. Tasks 5–9 are a **coordinated cutover**: intermediate tasks will NOT compile on their own; the first green build of the cutover comes at the **end of Task 9**. Do not run a full `colcon build` expecting success between Tasks 5 and 9 — only the lint/inspection steps shown.

---

## File Structure

**New files (framework):**
- `robot_description_setup_framework/include/robot_description_setup_framework/app_context.hpp` — `URDFModel` struct + `AppContext` holder (replaces `DataWarehouse`).
- `robot_description_setup_framework/include/robot_description_setup_framework/process_utils.hpp` + `src/process_utils.cpp` — `runProcess` fork/exec helper.
- `robot_description_setup_framework/include/robot_description_setup_framework/urdf_loader.hpp` + `src/urdf_loader.cpp` — xacro + urdfdom loader.
- `robot_description_setup_framework/include/robot_description_setup_framework/qt/joint_state_zero_publisher.hpp` + `src/joint_state_zero_publisher.cpp` — zero `JointState` C++ node.
- `robot_description_setup_framework/test/test_process_utils.cpp`, `test/test_urdf_loader.cpp`, `test/test_joint_state_zero_publisher.cpp`, `test/fixtures/simple_arm.urdf.xacro`.

**Rewritten files:**
- `robot_description_setup_framework/include/.../qt/rviz_panel.hpp` + `src/rviz_panel.cpp` — stock RobotModel display + embedded RSP/JSP, no MoveIt.
- `robot_description_setup_framework/include/.../setup_step.hpp`, `.../qt/setup_step_widget.hpp` — `AppContextPtr` instead of `DataWarehousePtr`.

**Modified files:**
- `robot_description_core_plugins/include/.../robot_selection.hpp` + `src/robot_selection.cpp` — drop moveit configs, add `loadRobot`.
- `robot_description_core_plugins/src/robot_selection_widget.cpp` — use `loadRobot` + `rviz_panel_->loadRobot`.
- `robot_description_core_plugins/src/start_screen_widget.cpp` — reword header text.
- `robot_description_setup_assistant/include/.../setup_assistant_widget.hpp` + `src/setup_assistant_widget.cpp` — `AppContext`, remove `unhighlightAll`, rework `onDataUpdate`.
- `robot_description_core_plugins/test/test_advanced_features.cpp` — new API.
- CMakeLists.txt + package.xml for `setup_framework` and `core_plugins`.

---

## Task 1: AppContext + URDFModel header and test harness

**Files:**
- Create: `robot_description_setup_framework/include/robot_description_setup_framework/app_context.hpp`
- Create: `robot_description_setup_framework/test/test_app_context.cpp`
- Modify: `robot_description_setup_framework/CMakeLists.txt`
- Modify: `robot_description_setup_framework/package.xml`

- [ ] **Step 1: Create the header**

Create `robot_description_setup_framework/include/robot_description_setup_framework/app_context.hpp`:

```cpp
#pragma once
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <rclcpp/node.hpp>

namespace robot_description
{
struct URDFModel
{
  std::string xml;                          // expanded URDF
  std::string robot_name;
  std::vector<std::string> movable_joints;  // non-fixed joints (for zero JointState)
  std::string root_link;
};

class AppContext
{
public:
  explicit AppContext(rclcpp::Node::SharedPtr node) : node_(std::move(node)) {}
  rclcpp::Node::SharedPtr getNode() const { return node_; }

  bool debug = false;
  std::optional<URDFModel> current_urdf;                   // last loaded
  std::optional<std::filesystem::path> preload_urdf_path;  // from --urdf_path

private:
  rclcpp::Node::SharedPtr node_;
};
using AppContextPtr = std::shared_ptr<AppContext>;
}  // namespace robot_description
```

- [ ] **Step 2: Write the failing test**

Create `robot_description_setup_framework/test/test_app_context.cpp`:

```cpp
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
```

- [ ] **Step 3: Wire gtest into CMake**

In `robot_description_setup_framework/CMakeLists.txt`, replace the existing `if(BUILD_TESTING)` block (lines ~64-67) with:

```cmake
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()

  find_package(ament_cmake_gtest REQUIRED)

  ament_add_gtest(test_app_context test/test_app_context.cpp)
  target_include_directories(test_app_context PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
  ament_target_dependencies(test_app_context rclcpp)
endif()
```

In `robot_description_setup_framework/package.xml`, add after the existing `<test_depend>` lines:

```xml
  <test_depend>ament_cmake_gtest</test_depend>
```

- [ ] **Step 4: Run test to verify it fails (before header) / passes (after)**

Run:
```bash
cd ~/rds_ws
colcon build --packages-select robot_description_setup_framework --event-handlers console_direct+
colcon test --packages-select robot_description_setup_framework --ctest-args -R test_app_context --event-handlers console_direct+
colcon test-result --verbose
```
Expected: build succeeds, `test_app_context` PASSES (1 test).

- [ ] **Step 5: Commit**

```bash
cd ~/rds_ws/src/robot-description-setup-assistant
git add robot_description_setup_framework/include/robot_description_setup_framework/app_context.hpp \
        robot_description_setup_framework/test/test_app_context.cpp \
        robot_description_setup_framework/CMakeLists.txt \
        robot_description_setup_framework/package.xml
git commit -m "feat(framework): add AppContext + URDFModel holder with gtest harness"
```

---

## Task 2: process_utils (runProcess)

**Files:**
- Create: `robot_description_setup_framework/include/robot_description_setup_framework/process_utils.hpp`
- Create: `robot_description_setup_framework/src/process_utils.cpp`
- Create: `robot_description_setup_framework/test/test_process_utils.cpp`
- Modify: `robot_description_setup_framework/CMakeLists.txt`

- [ ] **Step 1: Write the failing test**

Create `robot_description_setup_framework/test/test_process_utils.cpp`:

```cpp
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
  // sh -c writes to stderr
  int code = runProcess({ "sh", "-c", "echo oops 1>&2" }, out, err);
  EXPECT_EQ(code, 0);
  EXPECT_EQ(err, "oops\n");
}
```

- [ ] **Step 2: Run test to verify it fails**

Run:
```bash
cd ~/rds_ws
colcon build --packages-select robot_description_setup_framework --event-handlers console_direct+
```
Expected: FAIL — `process_utils.hpp` not found.

- [ ] **Step 3: Write the header**

Create `robot_description_setup_framework/include/robot_description_setup_framework/process_utils.hpp`:

```cpp
#pragma once
#include <string>
#include <vector>

namespace robot_description
{
// fork/exec argv (argv[0] resolved via PATH). Captures the child's stdout into
// `out` and stderr into `err`. Returns the child exit code, or -1 if the
// process could not be spawned. Does NOT use a shell (no injection risk).
int runProcess(const std::vector<std::string>& argv, std::string& out, std::string& err);
}  // namespace robot_description
```

- [ ] **Step 4: Write the implementation**

Create `robot_description_setup_framework/src/process_utils.cpp`:

```cpp
#include "robot_description_setup_framework/process_utils.hpp"

#include <array>
#include <cstring>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>

namespace robot_description
{
namespace
{
void drain(int fd, std::string& sink)
{
  std::array<char, 4096> buf;
  ssize_t n;
  while ((n = ::read(fd, buf.data(), buf.size())) > 0)
  {
    sink.append(buf.data(), static_cast<size_t>(n));
  }
}
}  // namespace

int runProcess(const std::vector<std::string>& argv, std::string& out, std::string& err)
{
  out.clear();
  err.clear();
  if (argv.empty())
  {
    return -1;
  }

  int out_pipe[2];
  int err_pipe[2];
  if (::pipe(out_pipe) != 0 || ::pipe(err_pipe) != 0)
  {
    return -1;
  }

  pid_t pid = ::fork();
  if (pid < 0)
  {
    return -1;
  }

  if (pid == 0)
  {
    // child
    ::dup2(out_pipe[1], STDOUT_FILENO);
    ::dup2(err_pipe[1], STDERR_FILENO);
    ::close(out_pipe[0]);
    ::close(out_pipe[1]);
    ::close(err_pipe[0]);
    ::close(err_pipe[1]);

    std::vector<char*> c_argv;
    c_argv.reserve(argv.size() + 1);
    for (const std::string& a : argv)
    {
      c_argv.push_back(const_cast<char*>(a.c_str()));
    }
    c_argv.push_back(nullptr);

    ::execvp(c_argv[0], c_argv.data());
    ::_exit(127);  // exec failed
  }

  // parent
  ::close(out_pipe[1]);
  ::close(err_pipe[1]);

  // Read both fds concurrently via poll to avoid pipe-buffer deadlock.
  std::array<pollfd, 2> fds{ { { out_pipe[0], POLLIN, 0 }, { err_pipe[0], POLLIN, 0 } } };
  int open_fds = 2;
  while (open_fds > 0)
  {
    int ready = ::poll(fds.data(), fds.size(), -1);
    if (ready < 0)
    {
      break;
    }
    for (auto& pfd : fds)
    {
      if (pfd.fd < 0)
      {
        continue;
      }
      if (pfd.revents & (POLLIN | POLLHUP))
      {
        std::array<char, 4096> buf;
        ssize_t n = ::read(pfd.fd, buf.data(), buf.size());
        if (n > 0)
        {
          (pfd.fd == out_pipe[0] ? out : err).append(buf.data(), static_cast<size_t>(n));
        }
        else  // EOF or error
        {
          ::close(pfd.fd);
          pfd.fd = -1;
          --open_fds;
        }
      }
    }
  }
  // ensure any straggler bytes are flushed
  if (fds[0].fd >= 0) { drain(out_pipe[0], out); ::close(out_pipe[0]); }
  if (fds[1].fd >= 0) { drain(err_pipe[0], err); ::close(err_pipe[0]); }

  int status = 0;
  ::waitpid(pid, &status, 0);
  if (WIFEXITED(status))
  {
    int code = WEXITSTATUS(status);
    return (code == 127) ? -1 : code;  // 127 == exec failed
  }
  return -1;
}
}  // namespace robot_description
```

- [ ] **Step 5: Add to library + test in CMake**

In `robot_description_setup_framework/CMakeLists.txt`, add `src/process_utils.cpp` to the `add_library(${PROJECT_NAME} ...)` source list (after `src/utilities.cpp`):

```cmake
add_library(${PROJECT_NAME}
  src/utilities.cpp
  src/process_utils.cpp
  src/helper_widgets.cpp
  src/rviz_panel.cpp
  ${MOC_FILES}
)
```

In the `if(BUILD_TESTING)` block, add after the `test_app_context` block:

```cmake
  ament_add_gtest(test_process_utils test/test_process_utils.cpp)
  target_include_directories(test_process_utils PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
  target_link_libraries(test_process_utils ${PROJECT_NAME})
```

- [ ] **Step 6: Run test to verify it passes**

Run:
```bash
cd ~/rds_ws
colcon build --packages-select robot_description_setup_framework --event-handlers console_direct+
colcon test --packages-select robot_description_setup_framework --ctest-args -R test_process_utils --event-handlers console_direct+
colcon test-result --verbose
```
Expected: PASS — 4 tests (`CapturesStdoutAndZeroExit`, `NonZeroExitCode`, `MissingBinaryReturnsNegative`, `CapturesStderr`).

- [ ] **Step 7: Commit**

```bash
cd ~/rds_ws/src/robot-description-setup-assistant
git add robot_description_setup_framework/include/robot_description_setup_framework/process_utils.hpp \
        robot_description_setup_framework/src/process_utils.cpp \
        robot_description_setup_framework/test/test_process_utils.cpp \
        robot_description_setup_framework/CMakeLists.txt
git commit -m "feat(framework): add runProcess fork/exec helper"
```

---

## Task 3: URDFLoader + URDFModel population

**Files:**
- Create: `robot_description_setup_framework/include/robot_description_setup_framework/urdf_loader.hpp`
- Create: `robot_description_setup_framework/src/urdf_loader.cpp`
- Create: `robot_description_setup_framework/test/test_urdf_loader.cpp`
- Create: `robot_description_setup_framework/test/fixtures/simple_arm.urdf.xacro`
- Modify: `robot_description_setup_framework/CMakeLists.txt`

- [ ] **Step 1: Create the test fixture**

Create `robot_description_setup_framework/test/fixtures/simple_arm.urdf.xacro`:

```xml
<?xml version="1.0"?>
<robot xmlns:xacro="http://www.ros.org/wiki/xacro" name="$(arg name)">
  <xacro:arg name="name" default="simple_arm"/>
  <link name="base_link"/>
  <link name="link1"/>
  <link name="link2"/>
  <joint name="joint1" type="revolute">
    <parent link="base_link"/>
    <child link="link1"/>
    <origin xyz="0 0 0.1"/>
    <axis xyz="0 0 1"/>
    <limit lower="-3.14" upper="3.14" effort="10" velocity="1"/>
  </joint>
  <joint name="fixed_joint" type="fixed">
    <parent link="link1"/>
    <child link="link2"/>
    <origin xyz="0 0 0.1"/>
  </joint>
</robot>
```

- [ ] **Step 2: Write the failing test**

Create `robot_description_setup_framework/test/test_urdf_loader.cpp`:

```cpp
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
  ASSERT_EQ(m.movable_joints.size(), 1u);          // fixed_joint excluded
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
```

- [ ] **Step 3: Run test to verify it fails**

Run:
```bash
cd ~/rds_ws
colcon build --packages-select robot_description_setup_framework --event-handlers console_direct+
```
Expected: FAIL — `urdf_loader.hpp` not found.

- [ ] **Step 4: Write the header**

Create `robot_description_setup_framework/include/robot_description_setup_framework/urdf_loader.hpp`:

```cpp
#pragma once
#include <filesystem>
#include <string>
#include "robot_description_setup_framework/app_context.hpp"  // URDFModel

namespace robot_description
{
class URDFLoader
{
public:
  // Throws std::runtime_error on: missing file, xacro non-zero exit, parse failure.
  static URDFModel load(const std::filesystem::path& urdf_path, const std::string& xacro_args);

private:
  static bool isXacro(const std::filesystem::path& p);
  static std::string runXacro(const std::filesystem::path& p, const std::string& args);
  static std::string readFile(const std::filesystem::path& p);
  static URDFModel parse(const std::string& xml);
};
}  // namespace robot_description
```

- [ ] **Step 5: Write the implementation**

Create `robot_description_setup_framework/src/urdf_loader.cpp`:

```cpp
#include "robot_description_setup_framework/urdf_loader.hpp"
#include "robot_description_setup_framework/process_utils.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <urdf_parser/urdf_parser.h>

namespace robot_description
{
bool URDFLoader::isXacro(const std::filesystem::path& p)
{
  const std::string s = p.string();
  return s.size() >= 6 && s.substr(s.size() - 6) == ".xacro";
}

std::string URDFLoader::readFile(const std::filesystem::path& p)
{
  std::ifstream in(p);
  if (!in)
  {
    throw std::runtime_error("Unable to open URDF file: " + p.string());
  }
  std::stringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

std::string URDFLoader::runXacro(const std::filesystem::path& p, const std::string& args)
{
  std::vector<std::string> argv{ "xacro", p.string() };
  // split args on whitespace ("name:=x ur_type:=ur5")
  std::istringstream iss(args);
  std::string tok;
  while (iss >> tok)
  {
    argv.push_back(tok);
  }

  std::string out, err;
  int code = runProcess(argv, out, err);
  if (code != 0)
  {
    throw std::runtime_error("xacro failed (exit " + std::to_string(code) + "): " + err);
  }
  return out;
}

URDFModel URDFLoader::parse(const std::string& xml)
{
  urdf::ModelInterfaceSharedPtr model = urdf::parseURDF(xml);
  if (!model)
  {
    throw std::runtime_error("URDF parse failed");
  }

  URDFModel out;
  out.xml = xml;
  out.robot_name = model->getName();
  if (model->getRoot())
  {
    out.root_link = model->getRoot()->name;
  }
  for (const auto& [name, joint] : model->joints_)
  {
    if (joint && joint->type != urdf::Joint::FIXED && joint->type != urdf::Joint::UNKNOWN)
    {
      out.movable_joints.push_back(name);
    }
  }
  return out;
}

URDFModel URDFLoader::load(const std::filesystem::path& urdf_path, const std::string& xacro_args)
{
  if (!std::filesystem::is_regular_file(urdf_path))
  {
    throw std::runtime_error("URDF file does not exist: " + urdf_path.string());
  }
  const std::string xml = isXacro(urdf_path) ? runXacro(urdf_path, xacro_args) : readFile(urdf_path);
  return parse(xml);
}
}  // namespace robot_description
```

- [ ] **Step 6: Wire into CMake**

In `robot_description_setup_framework/CMakeLists.txt`:

Add `src/urdf_loader.cpp` to the `add_library(${PROJECT_NAME} ...)` source list (after `src/process_utils.cpp`).

The `urdf` package is already a find_package + ament dependency — no change needed there.

In the `if(BUILD_TESTING)` block, add:

```cmake
  ament_add_gtest(test_urdf_loader test/test_urdf_loader.cpp)
  target_include_directories(test_urdf_loader PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
  target_compile_definitions(test_urdf_loader PRIVATE
    FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/fixtures")
  target_link_libraries(test_urdf_loader ${PROJECT_NAME})
  ament_target_dependencies(test_urdf_loader urdf)
```

- [ ] **Step 7: Run test to verify it passes**

Run:
```bash
cd ~/rds_ws
colcon build --packages-select robot_description_setup_framework --event-handlers console_direct+
source ~/rds_ws/install/setup.bash   # so `xacro` is on PATH
colcon test --packages-select robot_description_setup_framework --ctest-args -R test_urdf_loader --event-handlers console_direct+
colcon test-result --verbose
```
Expected: PASS — 2 tests. (Requires `xacro` on PATH; it ships with ROS desktop.)

- [ ] **Step 8: Commit**

```bash
cd ~/rds_ws/src/robot-description-setup-assistant
git add robot_description_setup_framework/include/robot_description_setup_framework/urdf_loader.hpp \
        robot_description_setup_framework/src/urdf_loader.cpp \
        robot_description_setup_framework/test/test_urdf_loader.cpp \
        robot_description_setup_framework/test/fixtures/simple_arm.urdf.xacro \
        robot_description_setup_framework/CMakeLists.txt
git commit -m "feat(framework): add URDFLoader (xacro + urdfdom introspection)"
```

---

## Task 4: JointStateZeroPublisher

**Files:**
- Create: `robot_description_setup_framework/include/robot_description_setup_framework/qt/joint_state_zero_publisher.hpp`
- Create: `robot_description_setup_framework/src/joint_state_zero_publisher.cpp`
- Create: `robot_description_setup_framework/test/test_joint_state_zero_publisher.cpp`
- Modify: `robot_description_setup_framework/CMakeLists.txt`
- Modify: `robot_description_setup_framework/package.xml`

- [ ] **Step 1: Write the header**

Create `robot_description_setup_framework/include/robot_description_setup_framework/qt/joint_state_zero_publisher.hpp`:

```cpp
#pragma once
#include <mutex>
#include <string>
#include <vector>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joint_state.hpp>

namespace robot_description::setup_framework
{
// Publishes a zero-position sensor_msgs/JointState for a set of joints at 10 Hz
// on topic "joint_states", so robot_state_publisher can compute TF for all links.
class JointStateZeroPublisher : public rclcpp::Node
{
public:
  explicit JointStateZeroPublisher(const rclcpp::NodeOptions& opts = rclcpp::NodeOptions());
  void setJoints(const std::vector<std::string>& names);

private:
  void onTimer();
  std::mutex mtx_;
  std::vector<std::string> names_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};
}  // namespace robot_description::setup_framework
```

- [ ] **Step 2: Write the failing test**

Create `robot_description_setup_framework/test/test_joint_state_zero_publisher.cpp`:

```cpp
#include <gtest/gtest.h>
#include <chrono>
#include <memory>
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
```

- [ ] **Step 3: Run test to verify it fails**

Run:
```bash
cd ~/rds_ws
colcon build --packages-select robot_description_setup_framework --event-handlers console_direct+
```
Expected: FAIL — `joint_state_zero_publisher.hpp` / `sensor_msgs` not found.

- [ ] **Step 4: Write the implementation**

Create `robot_description_setup_framework/src/joint_state_zero_publisher.cpp`:

```cpp
#include "robot_description_setup_framework/qt/joint_state_zero_publisher.hpp"

namespace robot_description::setup_framework
{
JointStateZeroPublisher::JointStateZeroPublisher(const rclcpp::NodeOptions& opts)
  : rclcpp::Node("joint_state_zero_publisher", opts)
{
  pub_ = create_publisher<sensor_msgs::msg::JointState>("joint_states", 10);
  timer_ = create_wall_timer(std::chrono::milliseconds(100), [this]() { onTimer(); });
}

void JointStateZeroPublisher::setJoints(const std::vector<std::string>& names)
{
  std::lock_guard<std::mutex> lock(mtx_);
  names_ = names;
}

void JointStateZeroPublisher::onTimer()
{
  std::lock_guard<std::mutex> lock(mtx_);
  if (names_.empty())
  {
    return;
  }
  sensor_msgs::msg::JointState msg;
  msg.header.stamp = now();
  msg.name = names_;
  msg.position.assign(names_.size(), 0.0);
  pub_->publish(msg);
}
}  // namespace robot_description::setup_framework
```

- [ ] **Step 5: Wire into CMake + package.xml**

In `robot_description_setup_framework/package.xml`, add to the `<depend>` block:

```xml
  <depend>sensor_msgs</depend>
```

In `robot_description_setup_framework/CMakeLists.txt`:

Add `find_package(sensor_msgs REQUIRED)` near the other find_package calls.

Add `src/joint_state_zero_publisher.cpp` to the `add_library(${PROJECT_NAME} ...)` source list.

Add `include/robot_description_setup_framework/qt/joint_state_zero_publisher.hpp` to the `qt5_wrap_cpp(MOC_FILES ...)` list.

Add `sensor_msgs` to `ament_target_dependencies(${PROJECT_NAME} ...)`.

Add `ament_export_dependencies(sensor_msgs)`.

In the `if(BUILD_TESTING)` block, add:

```cmake
  ament_add_gtest(test_joint_state_zero_publisher test/test_joint_state_zero_publisher.cpp)
  target_include_directories(test_joint_state_zero_publisher PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
  target_link_libraries(test_joint_state_zero_publisher ${PROJECT_NAME})
  ament_target_dependencies(test_joint_state_zero_publisher rclcpp sensor_msgs)
```

- [ ] **Step 6: Run test to verify it passes**

Run:
```bash
cd ~/rds_ws
colcon build --packages-select robot_description_setup_framework --event-handlers console_direct+
colcon test --packages-select robot_description_setup_framework --ctest-args -R test_joint_state_zero_publisher --event-handlers console_direct+
colcon test-result --verbose
```
Expected: PASS — 1 test.

- [ ] **Step 7: Commit**

```bash
cd ~/rds_ws/src/robot-description-setup-assistant
git add robot_description_setup_framework/include/robot_description_setup_framework/qt/joint_state_zero_publisher.hpp \
        robot_description_setup_framework/src/joint_state_zero_publisher.cpp \
        robot_description_setup_framework/test/test_joint_state_zero_publisher.cpp \
        robot_description_setup_framework/CMakeLists.txt \
        robot_description_setup_framework/package.xml
git commit -m "feat(framework): add JointStateZeroPublisher node"
```

---

## Task 5: Cutover — framework SetupStep / SetupStepWidget API (DataWarehouse → AppContext)

> **Cutover begins.** Tasks 5–8 will not compile until Task 9 is done. Each task ends with a **lint/visual inspection** step only, not a build. The first green build is at the end of Task 9.

**Files:**
- Modify: `robot_description_setup_framework/include/robot_description_setup_framework/setup_step.hpp`
- Modify: `robot_description_setup_framework/include/robot_description_setup_framework/qt/setup_step_widget.hpp`

- [ ] **Step 1: Rewrite `setup_step.hpp`**

Replace the MoveIt include and `DataWarehousePtr` usages. New full file body (keep the BSD header comment block at top unchanged; replace from `#pragma once` down):

```cpp
#pragma once
#include <rclcpp/node.hpp>
#include "robot_description_setup_framework/app_context.hpp"

namespace robot_description
{
class SetupStep
{
public:
  SetupStep() = default;
  SetupStep(const SetupStep&) = default;
  SetupStep(SetupStep&&) = default;
  SetupStep& operator=(const SetupStep&) = default;
  SetupStep& operator=(SetupStep&&) = default;
  virtual ~SetupStep() = default;

  void initialize(const rclcpp::Node::SharedPtr& parent_node, const AppContextPtr& context)
  {
    parent_node_ = parent_node;
    context_ = context;
    logger_ = std::make_shared<rclcpp::Logger>(parent_node->get_logger().get_child(getName()));
    onInit();
  }

  virtual void onInit() {}
  virtual std::string getName() const = 0;
  virtual bool isReady() const { return true; }

  const rclcpp::Logger& getLogger() const { return *logger_; }
  rclcpp::Node::SharedPtr getParentNode() const { return parent_node_; }
  AppContextPtr getContext() const { return context_; }

protected:
  AppContextPtr context_;
  rclcpp::Node::SharedPtr parent_node_;
  std::shared_ptr<rclcpp::Logger> logger_;
};
}  // namespace robot_description
```

- [ ] **Step 2: Rewrite `setup_step_widget.hpp` initialize signature**

In `robot_description_setup_framework/include/robot_description_setup_framework/qt/setup_step_widget.hpp`:

Replace the include `#include <moveit_setup_framework/data_warehouse.hpp>` — it is included transitively via `setup_step.hpp`; remove any direct moveit include. Then change the `initialize` method (lines ~79-94) to:

```cpp
  void initialize(const rclcpp::Node::SharedPtr& parent_node, QWidget* parent_widget, RVizPanel* rviz_panel,
                  const AppContextPtr& context)
  {
    getSetupStep().initialize(parent_node, context);
    setParent(parent_widget);
    rviz_panel_ = rviz_panel;
    debug_ = context->debug;
    onInit();

    RVizIntegratedWidget* rviz_widget = dynamic_cast<RVizIntegratedWidget*>(this);
    if (rviz_widget && rviz_widget->needsRVizPanel() && rviz_panel) {
      RCLCPP_INFO(rclcpp::get_logger("SetupStepWidget"), "Auto-integrating RViz panel for widget: %s",
                  getSetupStep().getName().c_str());
      rviz_widget->setRVizPanel(rviz_panel);
    }
  }
```

The header already includes `setup_step.hpp` (which brings `app_context.hpp`). Ensure no remaining `moveit_setup::` references in this file.

- [ ] **Step 3: Inspect (no build yet)**

Run:
```bash
grep -rn "moveit" robot_description_setup_framework/include/robot_description_setup_framework/setup_step.hpp \
                  robot_description_setup_framework/include/robot_description_setup_framework/qt/setup_step_widget.hpp
```
Expected: no matches.

- [ ] **Step 4: Commit**

```bash
cd ~/rds_ws/src/robot-description-setup-assistant
git add robot_description_setup_framework/include/robot_description_setup_framework/setup_step.hpp \
        robot_description_setup_framework/include/robot_description_setup_framework/qt/setup_step_widget.hpp
git commit -m "refactor(framework): SetupStep/SetupStepWidget use AppContext (part of MoveIt cutover)"
```

---

## Task 6: Cutover — rewrite RVizPanel (stock RobotModel + embedded RSP/JSP)

**Files:**
- Modify: `robot_description_setup_framework/include/robot_description_setup_framework/qt/rviz_panel.hpp`
- Modify: `robot_description_setup_framework/src/rviz_panel.cpp`

- [ ] **Step 1: Rewrite `rviz_panel.hpp`**

Replace from `#pragma once` down (keep BSD header comment) with:

```cpp
#pragma once

#include <thread>
#include <memory>

#include <rclcpp/rclcpp.hpp>
#include <rclcpp/logger.hpp>

#include <rviz_common/render_panel.hpp>
#include <rviz_common/window_manager_interface.hpp>
#include <rviz_common/visualization_manager.hpp>
#include <rviz_common/display.hpp>
#include <rviz_common/ros_integration/ros_node_abstraction_iface.hpp>

#include <robot_state_publisher/robot_state_publisher.hpp>

#include <QWidget>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>

#include "robot_description_setup_framework/app_context.hpp"
#include "robot_description_setup_framework/qt/joint_state_zero_publisher.hpp"

namespace robot_description::setup_framework
{
class RVizPanel : public QWidget, public rviz_common::WindowManagerInterface
{
  Q_OBJECT
public:
  RVizPanel(QWidget* parent,
            const rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr& node_abstraction,
            const AppContextPtr& context);

  RVizPanel(const RVizPanel&) = delete;
  RVizPanel& operator=(const RVizPanel&) = delete;
  RVizPanel(RVizPanel&&) = delete;
  RVizPanel& operator=(RVizPanel&&) = delete;
  ~RVizPanel() override;

  bool isInitialized() const { return rviz_render_panel_ != nullptr; }
  void initialize();
  void loadRobot(const URDFModel& model);

  QWidget* getParentWindow() override { return parent_; }
  rviz_common::PanelDockWidget* addPane(const QString&, QWidget*, Qt::DockWidgetArea = Qt::LeftDockWidgetArea,
                                        bool = true) override
  {
    return nullptr;
  }
  void setStatus(const QString&) override {}

protected:
  QWidget* parent_;
  rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr node_abstraction_;
  rclcpp::Node::SharedPtr node_;
  AppContextPtr context_;
  std::shared_ptr<rclcpp::Logger> logger_;

  std::unique_ptr<rviz_common::RenderPanel> rviz_render_panel_;
  std::unique_ptr<rviz_common::VisualizationManager> rviz_manager_;
  rviz_common::Display* robot_model_display_ = nullptr;

  std::shared_ptr<robot_state_publisher::RobotStatePublisher> rsp_node_;
  std::shared_ptr<JointStateZeroPublisher> jsp_node_;
  rclcpp::executors::SingleThreadedExecutor::SharedPtr exec_;
  std::thread spin_thread_;
};
}  // namespace robot_description::setup_framework
```

- [ ] **Step 2: Rewrite `rviz_panel.cpp`**

Replace from `#pragma once`/includes down (keep BSD header) with:

```cpp
#include "robot_description_setup_framework/qt/rviz_panel.hpp"

#include <rviz_rendering/render_window.hpp>
#include <rviz_common/view_manager.hpp>
#include <rviz_common/view_controller.hpp>
#include <rviz_common/tool_manager.hpp>
#include <rviz_common/display_group.hpp>

namespace robot_description::setup_framework
{
static const char* ROBOT_DESCRIPTION_TOPIC = "/robot_description";

RVizPanel::RVizPanel(QWidget* parent,
                     const rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr& node_abstraction,
                     const AppContextPtr& context)
  : QWidget(parent)
  , parent_(parent)
  , node_abstraction_(node_abstraction)
  , node_(node_abstraction_.lock()->get_raw_node())
  , context_(context)
{
  logger_ = std::make_shared<rclcpp::Logger>(node_->get_logger().get_child("RVizPanel"));
}

RVizPanel::~RVizPanel()
{
  if (exec_)
  {
    exec_->cancel();
  }
  if (spin_thread_.joinable())
  {
    spin_thread_.join();
  }
  rviz_manager_.reset();
  rviz_render_panel_.reset();
}

void RVizPanel::initialize()
{
  // ---- RViz render panel + manager ----
  rviz_render_panel_ = std::make_unique<rviz_common::RenderPanel>();
  rviz_render_panel_->setMinimumWidth(300);
  rviz_render_panel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  QApplication::processEvents();
  rviz_render_panel_->getRenderWindow()->initialize();

  rviz_manager_ = std::make_unique<rviz_common::VisualizationManager>(rviz_render_panel_.get(), node_abstraction_,
                                                                      this, node_->get_clock());
  rviz_render_panel_->initialize(rviz_manager_.get());
  rviz_manager_->initialize();
  rviz_manager_->startUpdate();
  rviz_manager_->getToolManager()->addTool("rviz_default_plugins/MoveCamera");

  // ---- Stock RobotModel display, sourced from /robot_description topic ----
  robot_model_display_ = rviz_manager_->createDisplay("rviz_default_plugins/RobotModel", "Robot", true);
  if (robot_model_display_)
  {
    // Property names verified against rviz_default_plugins (Humble).
    if (auto* p = robot_model_display_->subProp("Description Source"))
    {
      p->setValue("Topic");
    }
    if (auto* p = robot_model_display_->subProp("Description Topic"))
    {
      p->setValue(ROBOT_DESCRIPTION_TOPIC);
    }
  }

  rviz_common::ViewController* view = rviz_manager_->getViewManager()->getCurrent();
  view->subProp("Distance")->setValue(2.0f);

  // ---- Layout ----
  QVBoxLayout* rviz_layout = new QVBoxLayout();
  rviz_layout->addWidget(rviz_render_panel_.get());
  setLayout(rviz_layout);

  QHBoxLayout* btn_layout = new QHBoxLayout();
  rviz_layout->addLayout(btn_layout);
  QCheckBox* visual = new QCheckBox("visual");
  visual->setChecked(true);
  btn_layout->addWidget(visual);
  connect(visual, &QCheckBox::toggled, [this](bool checked) {
    if (robot_model_display_)
    {
      if (auto* p = robot_model_display_->subProp("Visual Enabled"))
      {
        p->setValue(checked);
      }
    }
  });
  QCheckBox* collision = new QCheckBox("collision");
  collision->setChecked(false);
  btn_layout->addWidget(collision);
  connect(collision, &QCheckBox::toggled, [this](bool checked) {
    if (robot_model_display_)
    {
      if (auto* p = robot_model_display_->subProp("Collision Enabled"))
      {
        p->setValue(checked);
      }
    }
  });

  // ---- Embedded RSP + JSP nodes on a background executor ----
  rclcpp::NodeOptions rsp_opts;
  rsp_opts.append_parameter_override("robot_description", std::string("<robot name=\"empty\"/>"));
  rsp_node_ = std::make_shared<robot_state_publisher::RobotStatePublisher>(rsp_opts);
  jsp_node_ = std::make_shared<JointStateZeroPublisher>();

  exec_ = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
  exec_->add_node(rsp_node_);
  exec_->add_node(jsp_node_);
  spin_thread_ = std::thread([this]() { exec_->spin(); });
}

void RVizPanel::loadRobot(const URDFModel& model)
{
  if (!rsp_node_ || !jsp_node_)
  {
    RCLCPP_ERROR(*logger_, "loadRobot called before initialize()");
    return;
  }
  rsp_node_->set_parameter(rclcpp::Parameter("robot_description", model.xml));
  jsp_node_->setJoints(model.movable_joints);

  if (rviz_manager_ && !model.root_link.empty())
  {
    rviz_manager_->setFixedFrame(QString::fromStdString(model.root_link));
  }
  if (robot_model_display_)
  {
    robot_model_display_->setEnabled(true);
  }
}
}  // namespace robot_description::setup_framework
```

- [ ] **Step 3: Inspect (no build yet)**

Run:
```bash
grep -rn "moveit" robot_description_setup_framework/include/robot_description_setup_framework/qt/rviz_panel.hpp \
                  robot_description_setup_framework/src/rviz_panel.cpp
```
Expected: no matches.

- [ ] **Step 4: Commit**

```bash
cd ~/rds_ws/src/robot-description-setup-assistant
git add robot_description_setup_framework/include/robot_description_setup_framework/qt/rviz_panel.hpp \
        robot_description_setup_framework/src/rviz_panel.cpp
git commit -m "refactor(framework): rewrite RVizPanel on stock RobotModel display + embedded RSP/JSP"
```

---

## Task 7: Cutover — core_plugins RobotSelection + widget

**Files:**
- Modify: `robot_description_core_plugins/include/robot_description_core_plugins/robot_selection.hpp`
- Modify: `robot_description_core_plugins/src/robot_selection.cpp`
- Modify: `robot_description_core_plugins/src/robot_selection_widget.cpp`
- Modify: `robot_description_core_plugins/src/start_screen_widget.cpp`

- [ ] **Step 1: Rewrite `robot_selection.hpp`**

Replace from `#pragma once` down (keep BSD header) with:

```cpp
#pragma once
#include <filesystem>
#include <rclcpp/rclcpp.hpp>
#include <robot_description_setup_framework/setup_step.hpp>
#include <robot_description_setup_framework/urdf_loader.hpp>

namespace robot_description::core_plugins
{
class RobotSelection : public SetupStep
{
public:
  std::string getName() const override { return "Robot Selection"; }
  bool isReady() const override { return true; }

  // Run the loader, store the result in AppContext, and return it.
  URDFModel loadRobot(const std::filesystem::path& urdf_path, const std::string& xacro_args)
  {
    URDFModel model = URDFLoader::load(urdf_path, xacro_args);
    if (auto ctx = getContext())
    {
      ctx->current_urdf = model;
    }
    return model;
  }
};
}  // namespace robot_description::core_plugins
```

- [ ] **Step 2: Simplify `robot_selection.cpp`**

Replace from `#include` down (keep BSD header) with:

```cpp
#include "robot_description_core_plugins/robot_selection.hpp"

// All logic is header-inline now; this translation unit keeps the build target
// stable and is a home for future non-inline methods.
namespace robot_description::core_plugins
{
}  // namespace robot_description::core_plugins
```

- [ ] **Step 3: Update `robot_selection_widget.cpp` — `loadDefinedFile`**

In `robot_description_core_plugins/src/robot_selection_widget.cpp`, replace the body of `loadDefinedFile` (the `try { setup_step_.loadURDFFile(...) ... }` block) so the success path uses the new API. The full method becomes:

```cpp
bool RobotSelectionWidget::loadDefinedFile(const RobotConfig& robot_config)
{
  std::filesystem::path urdf_path;

  try {
    urdf_path = robot_description::getSharePath(robot_config.urdf_package) / robot_config.urdf_path;
  } catch (const std::exception& e) {
    QMessageBox::warning(this, "Error Loading Files",
                        QString("Failed to locate package '%1': %2")
                        .arg(QString::fromStdString(robot_config.urdf_package))
                        .arg(e.what()));
    return false;
  }

  if (urdf_path.empty()) {
    QMessageBox::warning(this, "Error Loading Files", "No robot model file specified");
    return false;
  }

  if (!std::filesystem::is_regular_file(urdf_path)) {
    QMessageBox::warning(this, "Error Loading Files",
                         QString("Unable to locate the URDF file:\n%1\n\n"
                                "Please ensure the robot packages are properly installed.")
                         .arg(QString::fromStdString(urdf_path.string())));
    return false;
  }

  try {
    URDFModel model = setup_step_.loadRobot(urdf_path, robot_config.xacro_args);
    if (rviz_panel_) {
      rviz_panel_->loadRobot(model);
    }
    Q_EMIT dataUpdated();
    return true;
  } catch (const std::exception& e) {
    QMessageBox::critical(this, "Error Loading URDF",
                         QString("Failed to load URDF file:\n%1\n\nError details:\n%2")
                         .arg(QString::fromStdString(urdf_path.string()))
                         .arg(e.what()));
    return false;
  }
}
```

- [ ] **Step 4: Update `robot_selection_widget.cpp` — `loadAndUpdateVisualization`**

Replace the `if (loadDefinedFile(robot)) { ... rviz_panel_->updateFixedFrame() ... }` body of `loadAndUpdateVisualization` with:

```cpp
void RobotSelectionWidget::loadAndUpdateVisualization(const RobotConfig& robot)
{
  RCLCPP_INFO(setup_step_.getLogger(), "Loading URDF and updating visualization for robot: %s",
              robot.display_name.c_str());

  auto& config_manager = RobotConfigManager::getInstance();
  auto missing_packages = config_manager.getMissingPackages(robot);
  if (!missing_packages.empty()) {
    RCLCPP_WARN(setup_step_.getLogger(), "Robot has missing packages, showing validation dialog");
    showRobotValidationDialog(missing_packages);
    return;
  }

  // loadDefinedFile now performs both the URDF load and the RViz update.
  if (loadDefinedFile(robot)) {
    RCLCPP_INFO(setup_step_.getLogger(), "URDF loaded and visualization updated");
  } else {
    RCLCPP_ERROR(setup_step_.getLogger(), "Failed to load URDF for visualization");
  }
}
```

- [ ] **Step 5: Update `robot_selection_widget.cpp` — `showRobotValidationDialog` continue-anyway path**

In `showRobotValidationDialog`, the `else if (result == QMessageBox::Yes)` branch calls `loadDefinedFile(selected_robot_)` then `rviz_panel_->updateFixedFrame()`. Replace that branch body with (drop the `updateFixedFrame` call — `loadDefinedFile` handles RViz now):

```cpp
  } else if (result == QMessageBox::Yes) {
    RCLCPP_INFO(setup_step_.getLogger(), "User chose to continue with missing packages");
    loadDefinedFile(selected_robot_);  // also updates RViz on success
  } else {
```

- [ ] **Step 6: Reword `start_screen_widget.cpp` header text**

In `robot_description_core_plugins/src/start_screen_widget.cpp`, change the `HeaderWidget` description string (the line containing `"generate a MoveIt Configuration Package."`) to:

```cpp
                                        "The generated package can then be used directly in your ROS 2 robot "
                                        "description workflow.",
```

(Replace the two string-literal lines that mention MoveIt Setup Assistant with this single sentence.)

- [ ] **Step 7: Inspect (no build yet)**

Run:
```bash
grep -rn "moveit\|loadURDFFile\|updateFixedFrame\|srdf_config_\|package_settings_" \
  robot_description_core_plugins/src robot_description_core_plugins/include
```
Expected: no matches.

- [ ] **Step 8: Commit**

```bash
cd ~/rds_ws/src/robot-description-setup-assistant
git add robot_description_core_plugins/include/robot_description_core_plugins/robot_selection.hpp \
        robot_description_core_plugins/src/robot_selection.cpp \
        robot_description_core_plugins/src/robot_selection_widget.cpp \
        robot_description_core_plugins/src/start_screen_widget.cpp
git commit -m "refactor(core_plugins): RobotSelection uses URDFLoader + RVizPanel.loadRobot"
```

---

## Task 8: Cutover — setup_assistant_widget (AppContext)

**Files:**
- Modify: `robot_description_setup_assistant/include/robot_description_setup_assistant/setup_assistant_widget.hpp`
- Modify: `robot_description_setup_assistant/src/setup_assistant_widget.cpp`

- [ ] **Step 1: Update the header**

In `robot_description_setup_assistant/include/robot_description_setup_assistant/setup_assistant_widget.hpp`:

- Remove any direct moveit include (none expected; `config_data_` type comes from framework).
- Add include: `#include <robot_description_setup_framework/app_context.hpp>`
- Change the member declaration `moveit_setup::DataWarehousePtr config_data_;` (line ~117) to:

```cpp
  robot_description::AppContextPtr config_data_;
```

- The `pluginlib::ClassLoader<...SetupStepWidget>` line is unchanged.

- [ ] **Step 2: Update the constructor in `setup_assistant_widget.cpp`**

Replace the `config_data_` construction and debug/preload lines. Specifically:

Change:
```cpp
  config_data_ = std::make_shared<moveit_setup::DataWarehouse>(node_);

  if (args.count("debug"))
    config_data_->debug = true;
```
to:
```cpp
  config_data_ = std::make_shared<robot_description::AppContext>(node_);

  if (args.count("debug"))
    config_data_->debug = true;
```

Change the preload block:
```cpp
  if (args.count("urdf_path"))
  {
    config_data_->preloadWithURDFPath(args["urdf_path"].as<std::filesystem::path>());
  }
  if (args.count("config_pkg"))
  {
    config_data_->preloadWithFullConfig(args["config_pkg"].as<std::string>());
  }
```
to:
```cpp
  if (args.count("urdf_path"))
  {
    config_data_->preload_urdf_path = args["urdf_path"].as<std::filesystem::path>();
  }
  if (args.count("config_pkg"))
  {
    RCLCPP_INFO(node_->get_logger(), "Existing-config preload not yet supported: %s",
                args["config_pkg"].as<std::string>().c_str());
  }
```

- [ ] **Step 3: Initialize RViz eagerly + rework `onDataUpdate`**

In the constructor, after the `for` loop that adds setup-step widgets and before `QApplication::processEvents();` at the end, add an eager RViz init:

```cpp
  // Initialize RViz once; the RobotModel display renders from the
  // /robot_description topic, so no preloaded model is required.
  rviz_panel_->initialize();
```

Replace the entire `onDataUpdate()` method body with:

```cpp
void SetupRobotDescriptionAssistantWidget::onDataUpdate()
{
  for (size_t index = 0; index < steps_.size(); index++)
  {
    bool ready = steps_[index]->isReady();
    navs_view_->setEnabled(index, ready);
  }
}
```

- [ ] **Step 4: Remove `unhighlightAll` call in `moveToScreen`**

In `moveToScreen`, delete these two lines:
```cpp
    // Unhighlight anything on robot
    rviz_panel_->unhighlightAll();
```

- [ ] **Step 5: Inspect (no build yet)**

Run:
```bash
grep -rn "moveit\|unhighlightAll\|preloadWith\|isReadyForInitialization\|isRobotModelLoaded" \
  robot_description_setup_assistant/include robot_description_setup_assistant/src
```
Expected: no matches.

- [ ] **Step 6: Commit**

```bash
cd ~/rds_ws/src/robot-description-setup-assistant
git add robot_description_setup_assistant/include/robot_description_setup_assistant/setup_assistant_widget.hpp \
        robot_description_setup_assistant/src/setup_assistant_widget.cpp
git commit -m "refactor(assistant): use AppContext; init RViz eagerly; drop MoveIt gating"
```

---

## Task 9: Cutover finish — remove MoveIt from build files, add RSP deps, full green build

**Files:**
- Modify: `robot_description_setup_framework/CMakeLists.txt`
- Modify: `robot_description_setup_framework/package.xml`
- Modify: `robot_description_core_plugins/CMakeLists.txt`
- Modify: `robot_description_core_plugins/package.xml`

- [ ] **Step 1: framework `package.xml` — swap deps**

In `robot_description_setup_framework/package.xml`:
- Remove: `<depend>moveit_core</depend>`.
- Add (next to the other `<depend>` entries):
```xml
  <depend>robot_state_publisher</depend>
  <depend>tf2_ros</depend>
```
(`sensor_msgs` was already added in Task 4; `urdf`, `rviz_*` already present.)

- [ ] **Step 2: framework `CMakeLists.txt` — swap find_package + deps**

In `robot_description_setup_framework/CMakeLists.txt`:

Remove these lines:
```cmake
find_package(moveit_ros_planning REQUIRED)
find_package(moveit_ros_visualization REQUIRED)
find_package(moveit_setup_framework REQUIRED)
find_package(moveit_core REQUIRED)
```
Add:
```cmake
find_package(robot_state_publisher REQUIRED)
find_package(tf2_ros REQUIRED)
```

In `ament_target_dependencies(${PROJECT_NAME} ...)`, remove `moveit_core`, `moveit_ros_planning`, `moveit_ros_visualization`, `moveit_setup_framework`; add `robot_state_publisher`, `tf2_ros`. (`sensor_msgs` already added in Task 4.)

Remove these export lines:
```cmake
ament_export_dependencies(moveit_core)
ament_export_dependencies(moveit_ros_planning)
ament_export_dependencies(moveit_ros_visualization)
ament_export_dependencies(moveit_setup_framework)
```
Add:
```cmake
ament_export_dependencies(robot_state_publisher)
ament_export_dependencies(tf2_ros)
```

- [ ] **Step 3: core_plugins `package.xml` — drop moveit + srdfdom**

In `robot_description_core_plugins/package.xml`, remove:
```xml
  <depend>srdfdom</depend>
```
(There is no direct moveit `<depend>` here; moveit came transitively.)

- [ ] **Step 4: core_plugins `CMakeLists.txt` — drop moveit**

In `robot_description_core_plugins/CMakeLists.txt`:

Remove:
```cmake
find_package(moveit_ros_visualization REQUIRED)
find_package(moveit_setup_framework REQUIRED)
```
In `ament_target_dependencies(${PROJECT_NAME} ...)`, remove `moveit_ros_visualization` and `moveit_setup_framework`.

Remove these export lines:
```cmake
ament_export_dependencies(moveit_ros_visualization)
ament_export_dependencies(moveit_setup_framework)
```

- [ ] **Step 5: Full clean build of all three packages**

Run:
```bash
cd ~/rds_ws
rm -rf build/robot_description_setup_framework build/robot_description_core_plugins build/robot_description_setup_assistant \
       install/robot_description_setup_framework install/robot_description_core_plugins install/robot_description_setup_assistant
colcon build --packages-up-to robot_description_setup_assistant --event-handlers console_direct+
```
Expected: all three packages build green. No `moveit` symbols required.

- [ ] **Step 6: Verify MoveIt is gone**

Run:
```bash
cd ~/rds_ws/src/robot-description-setup-assistant
grep -rn "moveit" robot_description_setup_framework robot_description_core_plugins robot_description_setup_assistant \
  --include=*.cpp --include=*.hpp --include=*.txt --include=*.xml | grep -vi "MoveIt Setup Assistant"
```
Expected: no matches (other than possibly README prose, which is fine).

- [ ] **Step 7: Run the full framework test suite**

Run:
```bash
cd ~/rds_ws
colcon test --packages-select robot_description_setup_framework --event-handlers console_direct+
colcon test-result --verbose
```
Expected: `test_app_context`, `test_process_utils`, `test_urdf_loader`, `test_joint_state_zero_publisher` all PASS.

- [ ] **Step 8: Commit**

```bash
cd ~/rds_ws/src/robot-description-setup-assistant
git add robot_description_setup_framework/CMakeLists.txt robot_description_setup_framework/package.xml \
        robot_description_core_plugins/CMakeLists.txt robot_description_core_plugins/package.xml
git commit -m "build: remove MoveIt deps, add robot_state_publisher/tf2_ros; MoveIt-free build green"
```

---

## Task 10: Update manual test harness + integration verification

**Files:**
- Modify: `robot_description_core_plugins/test/test_advanced_features.cpp`

- [ ] **Step 1: Confirm the manual harness still compiles against new API**

`test_advanced_features.cpp` calls `widget->onInit()` only — no MoveIt API. Verify it still builds. If `RobotSelectionWidget::onInit()` now dereferences `getContext()` (via `setup_step_`), the harness must initialize the step with an `AppContext` first. Update `main` to:

```cpp
#include <QApplication>
#include <QMainWindow>
#include <rclcpp/rclcpp.hpp>
#include "robot_description_core_plugins/robot_selection_widget.hpp"
#include "robot_description_setup_framework/app_context.hpp"

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("test_advanced_features");

  QApplication app(argc, argv);
  QMainWindow window;
  window.setWindowTitle("Advanced Robot Selection Features Test");
  window.resize(1200, 800);

  auto context = std::make_shared<robot_description::AppContext>(node);
  auto* widget = new robot_description::core_plugins::RobotSelectionWidget();
  // initialize the underlying SetupStep with our context (no RViz panel in this harness)
  widget->initialize(node, &window, nullptr, context);

  window.setCentralWidget(widget);
  window.show();

  int result = app.exec();
  rclcpp::shutdown();
  return result;
}
```

> Note: `SetupStepWidget::initialize(node, parent, rviz_panel, context)` is the signature from Task 5. Passing `nullptr` for the RViz panel exercises the spec/info panel without 3D.

- [ ] **Step 2: Build the test executable**

Run:
```bash
cd ~/rds_ws
colcon build --packages-select robot_description_core_plugins --cmake-args -DBUILD_TESTING=ON --event-handlers console_direct+
```
Expected: `test_advanced_features` builds green.

- [ ] **Step 3: Manual integration check — launch the app**

Run:
```bash
cd ~/rds_ws
source ~/rds_ws/install/setup.bash
ros2 launch robot_description_setup_assistant setup_assistant.launch.py
```
Manual verification checklist:
- App opens, Start Screen shows (no MoveIt wording).
- Click "Create New" → Proceed → Robot Selection screen.
- Robot grid shows UR/KUKA cards.
- Toggle "Show 3D Visualization", click **UR5** → robot mesh renders in the RViz panel.
- Click **KUKA KR6 R700 Sixx** → view re-renders the new robot (live switch).
- No crash on screen switches.

(This requires UR/KUKA description packages installed, as today.)

- [ ] **Step 4: Commit**

```bash
cd ~/rds_ws/src/robot-description-setup-assistant
git add robot_description_core_plugins/test/test_advanced_features.cpp
git commit -m "test(core_plugins): update manual harness to AppContext init API"
```

---

## Self-Review Notes (for the implementer)

- **`updateFixedFrame` removed:** old callers were `loadAndUpdateVisualization` and `showRobotValidationDialog` (Task 7) and `onDataUpdate` (Task 8) — all updated. `RVizPanel::loadRobot` sets the fixed frame now.
- **`unhighlightAll` removed:** only caller was `moveToScreen` (Task 8 Step 4).
- **`DataWarehouse` removed everywhere:** framework headers (Task 5), rviz_panel (Task 6), setup_assistant (Task 8). `RobotSelection`'s old `package_settings_/srdf_config_/urdf_config_` dropped (Task 7).
- **RViz init timing change:** previously gated behind `isReadyForInitialization()` (needed a loaded MoveIt model). Now eager in the constructor (Task 8 Step 3) because the stock display renders from the topic.
- **Risk — RViz property names:** if `subProp("Description Source")` returns null on the target Humble build, the display already defaults to the `/robot_description` topic, so the null-guarded code degrades gracefully. Verify visually in Task 10 Step 3.
- **Risk — RSP constructor:** `robot_state_publisher::RobotStatePublisher(NodeOptions)` requires a non-empty `robot_description` param at construction in some distros; the placeholder `<robot name="empty"/>` in Task 6 Step 2 satisfies that. If construction still rejects it, set the param immediately after construction instead.
```
