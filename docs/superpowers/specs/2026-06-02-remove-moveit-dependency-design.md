# Remove MoveIt Dependency — Design Spec

- **Date:** 2026-06-02
- **Status:** Approved (pending written-spec review)
- **Branch:** `feat/remove-moveit-dependency`

## 1. Goal & Motivation

Remove all MoveIt dependencies from the Robot Description Setup Assistant (RDSA)
while retaining today's working functionality: **pick a robot from the YAML
catalog → expand its xacro into URDF → render it in an embedded RViz 3D view**,
with live robot-switching from the selection grid.

**Driver:** own the model layer. Replace MoveIt's setup framework with
home-grown C++ units so future custom features are not coupled to MoveIt's API
churn or install footprint.

**C++-first constraint:** everything in C++ where possible. The single
unavoidable non-C++ piece is the `xacro` CLI — no native C++ xacro expander
exists, and the UR / KUKA descriptions require xacro processing. It is invoked
as a subprocess (the same thing MoveIt's `URDFConfig` does internally). The
python `joint_state_publisher` is **not** used — it is replaced by a ~30-line
C++ node.

## 2. Current MoveIt Surface (what we remove)

Two distinct subsystems are in use today.

### A. Data / loading layer (`moveit_setup_framework`)
- `moveit_setup::DataWarehouse` / `DataWarehousePtr` — passed through
  `SetupStep`, `SetupStepWidget`, `RVizPanel`, `SetupRobotDescriptionAssistantWidget`.
- `URDFConfig` — `loadFromPath(path, xacro_args)`, `getURDFPath`, `getXacroArgs`,
  `isXacroFile`.
- `SRDFConfig` — `updateRobotModel()`, `getRobotModel()`.
- `PackageSettingsConfig` — `loadExisting(path)`, `getPackagePath()` (wired in
  `RobotSelection` but **never actually called**; start-screen load buttons are
  stubbed `QMessageBox`es).
- `DataWarehouse::preloadWithURDFPath` / `preloadWithFullConfig` / `.debug`.

### B. Model + render layer (`moveit_core` + `moveit_ros_visualization`)
- `moveit::core::RobotModel` / `LinkModel` / `JointModelGroup`.
- `moveit_rviz_plugin::RobotStateDisplay` — the widget that actually renders the
  robot in 3D.
- `RVizPanel::highlightLink/highlightGroup/unhighlightAll` — **dead code**; only
  `unhighlightAll()` is called (on screen switch in `moveToScreen`), and it is a
  no-op for the current screens.

**Key finding:** SRDF and `RobotModel` exist today *only* to feed
`RobotStateDisplay`. The stock RViz `RobotModel` display renders from raw URDF +
TF, so SRDF and the MoveIt model are not needed once we switch displays.

## 3. Replacement Map

| MoveIt thing removed | Replacement |
|---|---|
| `moveit_setup::DataWarehouse` | `AppContext` (plain holder: node, debug, current URDF, preload path) |
| `URDFConfig` / `SRDFConfig` / `PackageSettingsConfig` | `URDFLoader` + `URDFModel` (xacro + urdfdom) |
| `moveit::core::RobotModel` | dropped |
| `moveit_rviz_plugin::RobotStateDisplay` | `rviz_default_plugins/RobotModel` display |
| `highlightLink/Group/unhighlightAll` | dropped (dead code) |
| (new) | embedded `robot_state_publisher` C++ node |
| (new) | `JointStateZeroPublisher` C++ node (replaces python `joint_state_publisher`) |

## 4. New Units (each independently testable)

- **`process_utils`** — `runProcess(argv, &out, &err) -> int`. fork/exec/pipe, no
  shell (no injection). Captures stdout + stderr, returns exit code.
- **`URDFLoader` / `URDFModel`** — pure: xacro → xml → `urdf::Model` introspection
  → `{ xml, robot_name, movable_joints, root_link }`.
- **`AppContext`** — holder; replaces `DataWarehouse`.
- **`JointStateZeroPublisher`** — `rclcpp::Node`, publishes zero
  `sensor_msgs/msg/JointState` at 10 Hz for the movable joints.
- **`RVizPanel`** (rewrite) — owns the RViz render panel + visualization manager,
  the stock `RobotModel` display, the embedded RSP + JSP nodes, and a
  `SingleThreadedExecutor` spinning on a background thread.

## 5. Component Signatures / Skeletons

### `robot_description_setup_framework/app_context.hpp`
```cpp
#pragma once
#include <filesystem>
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

### `robot_description_setup_framework/process_utils.hpp`
```cpp
#pragma once
#include <string>
#include <vector>

namespace robot_description
{
// fork/exec argv (argv[0] resolved via PATH), capture stdout into `out` and
// stderr into `err`. Returns child exit code (-1 on spawn failure). No shell.
int runProcess(const std::vector<std::string>& argv, std::string& out, std::string& err);
}  // namespace robot_description
```

Implementation notes: `pipe()` x2, `fork()`, child `execvp`, parent reads both
pipes to EOF (avoid deadlock by reading in a loop / non-blocking or thread),
`waitpid`, return `WEXITSTATUS`.

### `robot_description_setup_framework/urdf_loader.hpp`
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
  // Throws std::runtime_error on: missing file, xacro non-zero exit, URDF parse failure.
  static URDFModel load(const std::filesystem::path& urdf_path, const std::string& xacro_args);

private:
  static bool isXacro(const std::filesystem::path& p);
  static std::string runXacro(const std::filesystem::path& p, const std::string& args);
  static std::string readFile(const std::filesystem::path& p);
  static URDFModel parse(const std::string& xml);  // urdf::Model.initString
};
}  // namespace robot_description
```

`runXacro`: split `args` ("name:=x ur_type:=ur5") on whitespace into tokens,
build argv `{"xacro", path, tok...}`, call `runProcess`, throw on non-zero.

`parse`: `urdf::Model m; if (!m.initString(xml)) throw;` →
`robot_name = m.getName()`, `root_link = m.getRoot()->name`, iterate
`m.joints_` collecting names where `type != urdf::Joint::FIXED`.

### `robot_description_setup_framework/qt/joint_state_zero_publisher.hpp`
```cpp
#pragma once
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joint_state.hpp>

namespace robot_description::setup_framework
{
class JointStateZeroPublisher : public rclcpp::Node
{
public:
  explicit JointStateZeroPublisher(const rclcpp::NodeOptions& opts = {});
  void setJoints(const std::vector<std::string>& names);  // thread-safe

private:
  void onTimer();
  std::mutex mtx_;
  std::vector<std::string> names_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr pub_;  // topic "joint_states"
  rclcpp::TimerBase::SharedPtr timer_;  // 10 Hz
};
}  // namespace robot_description::setup_framework
```

### `robot_description_setup_framework/qt/rviz_panel.hpp` (rewrite — no MoveIt)
```cpp
#pragma once
#include <thread>
#include <rclcpp/rclcpp.hpp>
#include <rviz_common/render_panel.hpp>
#include <rviz_common/visualization_manager.hpp>
#include <rviz_common/window_manager_interface.hpp>
#include <robot_state_publisher/robot_state_publisher.hpp>
#include <QWidget>
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
  ~RVizPanel() override;

  RVizPanel(const RVizPanel&) = delete;
  RVizPanel& operator=(const RVizPanel&) = delete;

  bool isInitialized() const { return rviz_render_panel_ != nullptr; }
  void initialize();                 // build rviz + RobotModel display + start spin thread
  void loadRobot(const URDFModel& model);  // set RSP param + joints + fixed frame

  // WindowManagerInterface
  QWidget* getParentWindow() override { return parent_; }
  rviz_common::PanelDockWidget* addPane(const QString&, QWidget*, Qt::DockWidgetArea, bool) override
  { return nullptr; }
  void setStatus(const QString&) override {}

private:
  QWidget* parent_;
  rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr node_abstraction_;
  rclcpp::Node::SharedPtr node_;
  AppContextPtr context_;

  std::unique_ptr<rviz_common::RenderPanel> rviz_render_panel_;
  std::unique_ptr<rviz_common::VisualizationManager> rviz_manager_;
  rviz_common::Display* robot_model_display_ = nullptr;  // rviz_default_plugins/RobotModel

  std::shared_ptr<robot_state_publisher::RobotStatePublisher> rsp_node_;
  std::shared_ptr<JointStateZeroPublisher> jsp_node_;
  rclcpp::executors::SingleThreadedExecutor::SharedPtr exec_;
  std::thread spin_thread_;
};
}  // namespace robot_description::setup_framework
```

`initialize()`:
- build `RenderPanel`, `VisualizationManager` (as today), add MoveCamera tool;
- `robot_model_display_ = rviz_manager_->createDisplay("rviz_default_plugins/RobotModel", "Robot", true);`
- `robot_model_display_->subProp("Description Source")->setValue("Topic");`
  `robot_model_display_->subProp("Description Topic")->setValue("/robot_description");`
  *(verify exact property names in Humble at impl; fall back to topic default if "Source" absent)*
- construct `rsp_node_` (NodeOptions; empty robot_description initially),
  `jsp_node_`; add both to `exec_`; `spin_thread_ = std::thread([this]{ exec_->spin(); });`

`loadRobot(model)`:
- `rsp_node_->set_parameter(rclcpp::Parameter("robot_description", model.xml));`
  *(RSP republishes latched `/robot_description` + recomputes TF on param change)*
- `jsp_node_->setJoints(model.movable_joints);`
- `rviz_manager_->setFixedFrame(QString::fromStdString(model.root_link));`

`~RVizPanel()`: `exec_->cancel(); if (spin_thread_.joinable()) spin_thread_.join();`
then reset rviz objects.

## 6. Changes to Existing Units

### `setup_step.hpp` / `setup_step_widget.hpp`
- `initialize(node, AppContextPtr)` replaces `DataWarehousePtr`.
- `getConfigData()` returns `AppContextPtr`.
- `SetupStepWidget::initialize` takes `AppContextPtr`; `debug_ = config->debug`.
- `RVizIntegratedWidget` interface unchanged.

### `core_plugins/robot_selection.hpp/.cpp`
- Drop `package_settings_` / `srdf_config_` / `urdf_config_` members and the
  three moveit includes.
- New:
  ```cpp
  URDFModel loadRobot(const std::filesystem::path& urdf_path, const std::string& xacro_args);
  ```
  body: `auto m = URDFLoader::load(urdf_path, xacro_args); getConfigData()->current_urdf = m; return m;`
- `onInit()` becomes empty (or caches context).

### `core_plugins/robot_selection_widget.cpp`
- `loadDefinedFile`: resolve path as today, then
  `URDFModel m = setup_step_.loadRobot(urdf_path, robot_config.xacro_args);`
  store, `Q_EMIT dataUpdated();`.
- `loadAndUpdateVisualization`: replace `rviz_panel_->updateFixedFrame()` with
  `rviz_panel_->loadRobot(*context->current_urdf)` (or pass model directly).
- Keep missing-ROS-package validation dialog (ament index) unchanged.

### `core_plugins/start_screen_widget.cpp`
- Reword the header text away from "generate a MoveIt Configuration Package"
  (cosmetic). Stubbed load buttons stay stubbed.

### `setup_assistant/setup_assistant_widget.hpp/.cpp`
- `config_data_` type → `AppContextPtr`;
  `config_data_ = std::make_shared<AppContext>(node_);`.
- `--urdf_path` → `config_data_->preload_urdf_path = ...`; `--config_pkg` →
  logged/no-op for now (existing-package flow already stubbed).
- `moveToScreen`: remove `rviz_panel_->unhighlightAll()` call.
- `onDataUpdate`: remove `isReadyForInitialization` / `isRobotModelLoaded`
  gating; initialize RViz eagerly (once) since the display renders from the
  `/robot_description` topic, not a preloaded model. Init can happen at
  construction after the panel is added, or on first `dataUpdated`.

## 7. Data Flow (pick robot → 3D)

1. Card click → `onRobotSelected` → (viz mode) `loadAndUpdateVisualization`.
2. `loadDefinedFile` → `getSharePath(urdf_package)/urdf_path` →
   `RobotSelection::loadRobot` → `URDFLoader::load`:
   - `isXacro` → `runXacro` (fork/exec `xacro <path> <args>` → stdout) else `readFile`;
   - `urdf::Model.initString(xml)` → robot_name, root_link, movable_joints;
   - returns `URDFModel`; stored in `AppContext::current_urdf`.
3. `RVizPanel::loadRobot(model)`:
   - set RSP `robot_description` param → RSP republishes latched
     `/robot_description` + recomputes TF;
   - `jsp_node_->setJoints(...)` → zero `JointState` → RSP emits TF for all links;
   - `setFixedFrame(root_link)`;
   - stock `RobotModel` display re-reads topic, renders meshes via TF.
4. Background executor thread keeps RSP + JSP spinning. RViz uses its own node's
   TF buffer; cross-node topics travel via DDS loopback.

Live robot switch = repeat 2–3 (update param + joints). No relaunch.

## 8. Error Handling

- `runProcess` non-zero exit → `runXacro` throws `std::runtime_error(stderr)`.
- `urdf::Model.initString` false → throw `std::runtime_error("URDF parse failed")`.
- Missing file → throw before exec.
- `RobotSelectionWidget::loadDefinedFile` keeps its `try/catch` → `QMessageBox`
  (existing pattern).
- RSP param-set failure → log + `QMessageBox`.
- Existing missing-ROS-package validation dialog (ament index) unchanged.

## 9. Build / Dependency Changes

### `robot_description_setup_framework`
- **Drop** find_package + depend: `moveit_core`, `moveit_ros_planning`,
  `moveit_ros_visualization`, `moveit_setup_framework`.
- **Add**: `robot_state_publisher`, `sensor_msgs`, `tf2_ros`.
- **Keep**: `urdf`, `rviz_common`, `rviz_rendering`, `rviz_default_plugins`,
  `pluginlib`, `rclcpp`, `Qt5`, `ament_index_cpp`.
- `qt5_wrap_cpp`: add `qt/joint_state_zero_publisher.hpp`; keep `rviz_panel.hpp`,
  `setup_step_widget.hpp`, `helper_widgets.hpp`.
- New sources: `process_utils.cpp`, `urdf_loader.cpp`,
  `joint_state_zero_publisher.cpp`.

### `robot_description_core_plugins`
- **Drop**: `moveit_ros_visualization`, `moveit_setup_framework`, `srdfdom`.
- **Keep**: `urdf`, `yaml-cpp`, `robot_description_setup_framework`, etc.

### `robot_description_setup_assistant`
- No MoveIt (only transitive today). No dep changes beyond rebuild against the
  new framework API.

## 10. Testing

- **`process_utils`**: `runProcess({"echo","hi"}, out, err)` → `out=="hi\n"`, ret 0;
  bad binary → ret -1.
- **`URDFLoader::load`**: fixture `.xacro` (small 2-joint arm) → assert
  `robot_name`, `movable_joints.size()`, `xml` non-empty; non-existent path →
  throws; malformed xml → throws. (No MoveIt needed to build tests anymore.)
- **`JointStateZeroPublisher`**: `setJoints({"a","b"})`, spin once, capture
  message → names match, all positions 0.
- **Manual / integration**: update `test_advanced_features.cpp` to the new API;
  `ros2 launch ... setup_assistant.launch.py`, pick UR5 → robot renders in 3D;
  switch to KUKA KR6 → re-renders.

## 11. Known Risks (resolve at implementation)

- Exact RViz `RobotModel` display property strings in Humble
  (`Description Source` / `Description Topic`). Mitigation: verify against
  `rviz_default_plugins`; if "Source" property is absent, the display already
  defaults to the `/robot_description` topic.
- `robot_state_publisher::RobotStatePublisher` exported target / header path.
  Confirm the component lib is linkable (`robot_state_publisher` exports its
  node as a component in Humble).
- Reading two child pipes (stdout+stderr) without deadlock — read both to EOF
  via `poll`/threads, not sequential blocking reads on full buffers.

## 12. Out of Scope (YAGNI)

- urdfdom-based rich model object beyond joint/link enumeration.
- Own SRDF / planning groups / link-highlight revival.
- Existing-package load + config-package generation (already stubbed; not part
  of today's working functionality).
