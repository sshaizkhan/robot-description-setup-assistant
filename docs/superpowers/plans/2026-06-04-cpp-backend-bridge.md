# C++ Catalog Backend Bridge — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the existing C++ catalog logic the source of truth for the web UI: a pure-C++ `RobotCatalogCore` (de-Qt) wrapped in a `rclcpp` service node, bridged into the FastAPI backend via `rclpy`, with the React frontend and HTTP contract unchanged.

**Architecture:** Approach A from the spec. `RobotCatalogCore` (no Qt) holds the catalog + filter/search/validate logic ported from `RobotConfigManager`. `robot_catalog_server` (rclcpp) advertises services; responses are hand-written JSON strings (no nlohmann dep), filter input uses typed `.srv` fields. FastAPI's catalog/validate endpoints call those services with rclpy and forward JSON; everything else (urdf/meshes/ws/image/package) stays Python.

**Tech Stack:** C++17, ament_cmake, rclcpp, yaml-cpp (existing), ament_cmake_gtest; rosidl `.srv`; Python FastAPI + rclpy; React (unchanged).

---

## Substrate facts

- Branch `feat/cpp-backend-bridge` is already created off `master` (C++ packages intact).
- The web stack lives on `feat/webui-rewrite` under `web/` and `robot_description_setup_assistant/launch/webui.launch.py`.
- `RobotConfigManager` source to port: `robot_description_core_plugins/src/robot_config_manager.cpp` (629 lines) + its header (195). Structs `RobotConfig`, `RobotSpecifications`, `CategoryInfo`, `RobotFilter` are plain value types; the Qt parts are `QObject`, `QFileSystemWatcher`, `Q_SIGNALS`/slots, and `QString` in signal signatures.
- ROS Humble, yaml-cpp via `yaml_cpp_vendor`. No `nlohmann/json` — we hand-write JSON output and use typed `.srv` input.

## Package layout (new + modified)

```
robot_catalog_core/                 # NEW: pure C++ library (no Qt, no ROS)
  package.xml, CMakeLists.txt
  include/robot_catalog_core/types.hpp        # RobotConfig/Specs/Category/Filter
  include/robot_catalog_core/catalog.hpp      # RobotCatalogCore
  include/robot_catalog_core/json.hpp         # tiny JSON writer
  src/catalog.cpp
  src/json.cpp
  test/test_catalog.cpp                       # gtest parity suite

robot_catalog_msgs/                 # NEW: service definitions
  package.xml, CMakeLists.txt
  srv/GetRobots.srv  GetCategories.srv  FilterRobots.srv  ValidateRobot.srv

robot_catalog_server/               # NEW: rclcpp node
  package.xml, CMakeLists.txt
  src/server_node.cpp

web/                                # BROUGHT OVER from feat/webui-rewrite
  backend/rdsa_web/catalog_client.py          # NEW: rclpy service client
  backend/rdsa_web/app.py                      # MODIFY: use catalog_client
  ...
robot_description_setup_assistant/launch/webui.launch.py   # MODIFY: start the C++ node
```

---

## PHASE 1 — Substrate

### Task 1.1: Bring the web stack onto this branch

**Files:** adds `web/` and `webui.launch.py` from `feat/webui-rewrite`.

- [ ] **Step 1: Check out the web tree + launch file from the other branch**

```bash
cd /home/bot/rds_ws/src/robot-description-setup-assistant
git checkout feat/webui-rewrite -- web
git checkout feat/webui-rewrite -- robot_description_setup_assistant/launch/webui.launch.py
git checkout feat/webui-rewrite -- .gitignore
```

- [ ] **Step 2: Verify the backend tests still pass (YAML-backed, no C++ yet)**

```bash
cd web/backend
[ -x .venv/bin/pytest ] || { uv venv --system-site-packages --python 3.10 .venv && . .venv/bin/activate && uv pip install -e ".[dev]"; }
. .venv/bin/activate
RDSA_ROBOTS_YAML="$(cd ../.. && pwd)/robot_description_setup_assistant/config/robots.yaml" pytest tests/ -q
```
Expected: all backend tests pass (the Python YAML path is unchanged at this point).

- [ ] **Step 3: Commit**

```bash
cd /home/bot/rds_ws/src/robot-description-setup-assistant
git add web robot_description_setup_assistant/launch/webui.launch.py .gitignore
git commit -m "chore: bring web stack onto cpp-bridge branch (from webui-rewrite)"
```

---

## PHASE 2 — `robot_catalog_core` (pure C++ + gtest)

### Task 2.1: Package skeleton + value types

**Files:**
- Create: `robot_catalog_core/package.xml`, `robot_catalog_core/CMakeLists.txt`
- Create: `robot_catalog_core/include/robot_catalog_core/types.hpp`

- [ ] **Step 1: `package.xml`**

```xml
<?xml version="1.0"?>
<package format="3">
  <name>robot_catalog_core</name>
  <version>0.1.0</version>
  <description>Pure C++ robot catalog: load/filter/search/validate (no Qt, no ROS).</description>
  <maintainer email="sshaizkhan@gmail.com">Shahwaz Khan</maintainer>
  <license>BSD</license>
  <buildtool_depend>ament_cmake</buildtool_depend>
  <depend>yaml_cpp_vendor</depend>
  <depend>ament_index_cpp</depend>
  <test_depend>ament_cmake_gtest</test_depend>
  <export><build_type>ament_cmake</build_type></export>
</package>
```

- [ ] **Step 2: `types.hpp` (ported from robot_config_manager.hpp, Qt removed)**

```cpp
#pragma once
#include <string>
#include <vector>
#include <optional>

namespace robot_catalog {

struct CategoryInfo {
  std::string id, display_name, description, manufacturer, website;
};

struct RobotSpecifications {
  int degrees_of_freedom = 0;
  double payload_kg = 0.0, reach_mm = 0.0, weight_kg = 0.0;
  double repeatability_mm = 0.0, max_speed_ms = 0.0;
  std::vector<std::string> mounting_options;
  bool safety_certified = false, collaborative = false, torque_sensing = false;
};

struct RobotConfig {
  std::string id, display_name, description, image_path;
  std::string urdf_package, urdf_path, xacro_args, category;
  RobotSpecifications specifications;
  std::vector<std::string> required_packages, optional_packages, tags;
};

struct RobotFilter {
  std::optional<std::string> category;
  std::optional<double> min_payload, max_payload, min_reach, max_reach;
  std::optional<int> degrees_of_freedom;
  std::optional<bool> collaborative_only;
  std::vector<std::string> required_tags;
  std::string search_text;
  bool isEmpty() const {
    return !category && !min_payload && !max_payload && !min_reach &&
           !max_reach && !degrees_of_freedom && !collaborative_only &&
           required_tags.empty() && search_text.empty();
  }
};

}  // namespace robot_catalog
```

- [ ] **Step 3: `CMakeLists.txt` (library + test wiring; test added in 2.4)**

```cmake
cmake_minimum_required(VERSION 3.10)
project(robot_catalog_core)
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
find_package(ament_cmake REQUIRED)
find_package(yaml_cpp_vendor REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(yaml-cpp REQUIRED)

add_library(robot_catalog_core src/catalog.cpp src/json.cpp)
target_include_directories(robot_catalog_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
ament_target_dependencies(robot_catalog_core ament_index_cpp)
target_link_libraries(robot_catalog_core yaml-cpp)

install(DIRECTORY include/ DESTINATION include)
install(TARGETS robot_catalog_core EXPORT export_robot_catalog_core
  ARCHIVE DESTINATION lib LIBRARY DESTINATION lib RUNTIME DESTINATION bin)
ament_export_targets(export_robot_catalog_core HAS_LIBRARY_TARGET)
ament_export_dependencies(ament_index_cpp)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_catalog test/test_catalog.cpp)
  target_link_libraries(test_catalog robot_catalog_core yaml-cpp)
endif()
ament_package()
```

- [ ] **Step 4: Commit**

```bash
git add robot_catalog_core/package.xml robot_catalog_core/CMakeLists.txt robot_catalog_core/include
git commit -m "feat(catalog-core): package skeleton + value types"
```

### Task 2.2: JSON writer

**Files:**
- Create: `robot_catalog_core/include/robot_catalog_core/json.hpp`, `src/json.cpp`

- [ ] **Step 1: `json.hpp`**

```cpp
#pragma once
#include <string>
#include <vector>
#include "robot_catalog_core/types.hpp"

namespace robot_catalog {
std::string json_escape(const std::string& s);
std::string to_json(const CategoryInfo& c);
std::string to_json(const RobotConfig& r);
std::string robots_to_json(const std::vector<RobotConfig>& robots);
std::string categories_to_json(const std::vector<CategoryInfo>& cats);
}  // namespace robot_catalog
```

- [ ] **Step 2: `src/json.cpp` (minimal, dependency-free)**

```cpp
#include "robot_catalog_core/json.hpp"
#include <sstream>

namespace robot_catalog {

std::string json_escape(const std::string& s) {
  std::string out;
  out.reserve(s.size() + 8);
  for (char c : s) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          char buf[8];
          std::snprintf(buf, sizeof(buf), "\\u%04x", c);
          out += buf;
        } else {
          out += c;
        }
    }
  }
  return out;
}

static std::string q(const std::string& s) { return "\"" + json_escape(s) + "\""; }
static std::string b(bool v) { return v ? "true" : "false"; }

static std::string str_array(const std::vector<std::string>& v) {
  std::string out = "[";
  for (size_t i = 0; i < v.size(); ++i) {
    if (i) out += ",";
    out += q(v[i]);
  }
  return out + "]";
}

static std::string num(double v) {
  std::ostringstream os;
  os << v;
  return os.str();
}

static std::string specs_json(const RobotSpecifications& s) {
  std::string out = "{";
  out += "\"degrees_of_freedom\":" + std::to_string(s.degrees_of_freedom);
  out += ",\"payload_kg\":" + num(s.payload_kg);
  out += ",\"reach_mm\":" + num(s.reach_mm);
  out += ",\"weight_kg\":" + num(s.weight_kg);
  out += ",\"repeatability_mm\":" + num(s.repeatability_mm);
  out += ",\"max_speed_ms\":" + num(s.max_speed_ms);
  out += ",\"mounting_options\":" + str_array(s.mounting_options);
  out += ",\"safety_certified\":" + b(s.safety_certified);
  out += ",\"collaborative\":" + b(s.collaborative);
  out += ",\"torque_sensing\":" + b(s.torque_sensing);
  return out + "}";
}

std::string to_json(const RobotConfig& r) {
  std::string out = "{";
  out += "\"id\":" + q(r.id);
  out += ",\"display_name\":" + q(r.display_name);
  out += ",\"description\":" + q(r.description);
  out += ",\"image_path\":" + q(r.image_path);
  out += ",\"urdf_package\":" + q(r.urdf_package);
  out += ",\"urdf_path\":" + q(r.urdf_path);
  out += ",\"xacro_args\":" + q(r.xacro_args);
  out += ",\"category\":" + q(r.category);
  out += ",\"specifications\":" + specs_json(r.specifications);
  out += ",\"required_packages\":" + str_array(r.required_packages);
  out += ",\"optional_packages\":" + str_array(r.optional_packages);
  out += ",\"tags\":" + str_array(r.tags);
  return out + "}";
}

std::string to_json(const CategoryInfo& c) {
  std::string out = "{";
  out += "\"id\":" + q(c.id);
  out += ",\"display_name\":" + q(c.display_name);
  out += ",\"description\":" + q(c.description);
  out += ",\"manufacturer\":" + q(c.manufacturer);
  out += ",\"website\":" + q(c.website);
  return out + "}";
}

std::string robots_to_json(const std::vector<RobotConfig>& robots) {
  std::string out = "[";
  for (size_t i = 0; i < robots.size(); ++i) {
    if (i) out += ",";
    out += to_json(robots[i]);
  }
  return out + "]";
}

std::string categories_to_json(const std::vector<CategoryInfo>& cats) {
  std::string out = "[";
  for (size_t i = 0; i < cats.size(); ++i) {
    if (i) out += ",";
    out += to_json(cats[i]);
  }
  return out + "]";
}

}  // namespace robot_catalog
```

- [ ] **Step 3: Commit**

```bash
git add robot_catalog_core/include/robot_catalog_core/json.hpp robot_catalog_core/src/json.cpp
git commit -m "feat(catalog-core): dependency-free JSON writer"
```

### Task 2.3: `RobotCatalogCore` (load/filter/search/validate)

**Files:**
- Create: `robot_catalog_core/include/robot_catalog_core/catalog.hpp`, `src/catalog.cpp`

- [ ] **Step 1: `catalog.hpp`**

```cpp
#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "robot_catalog_core/types.hpp"

namespace robot_catalog {

class RobotCatalogCore {
public:
  // Throws std::runtime_error on YAML/IO error.
  void loadFromFile(const std::string& path);

  std::vector<RobotConfig> getAllRobots() const;
  bool getRobotById(const std::string& id, RobotConfig& out) const;
  std::vector<CategoryInfo> getCategories() const;
  std::vector<RobotConfig> getRobotsByCategory(const std::string& category) const;
  std::vector<RobotConfig> filterRobots(const RobotFilter& f) const;
  std::vector<RobotConfig> searchRobots(const std::string& term) const;
  std::vector<std::string> getMissingPackages(const RobotConfig& r) const;
  bool validateRobotPackages(const RobotConfig& r) const;

private:
  static bool matchesFilter(const RobotConfig& r, const RobotFilter& f);
  static bool matchesSearch(const RobotConfig& r, const std::string& term);
  std::unordered_map<std::string, RobotConfig> robots_;
  std::unordered_map<std::string, CategoryInfo> categories_;
};

}  // namespace robot_catalog
```

- [ ] **Step 2: `src/catalog.cpp`**

Port the YAML parsing from `robot_description_core_plugins/src/robot_config_manager.cpp`
(`parseRobotConfig`, `parseCategoryInfo`, `parseSpecifications`, `mergeConfiguration`)
and the predicates (`matchesFilter`, `matchesSearchTerm`) — these are already plain
yaml-cpp / std code. The filter/search semantics MUST match the Python backend
(`web/backend/rdsa_web/catalog.py`): category equality; payload/reach inclusive
min/max; dof equality; `collaborative_only` true ⇒ require collaborative; all
`required_tags` present; search case-insensitive over display_name, description,
category, tags. Missing-package check uses
`ament_index_cpp::get_package_share_directory` (throws when missing). YAML key
`mounting` maps to `mounting_options`.

```cpp
#include "robot_catalog_core/catalog.hpp"
#include <algorithm>
#include <stdexcept>
#include <yaml-cpp/yaml.h>
#include <ament_index_cpp/get_package_share_directory.hpp>

namespace robot_catalog {

static std::string lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), ::tolower);
  return s;
}

static RobotSpecifications parseSpecs(const YAML::Node& n) {
  RobotSpecifications s;
  if (!n) return s;
  s.degrees_of_freedom = n["degrees_of_freedom"].as<int>(0);
  s.payload_kg = n["payload_kg"].as<double>(0.0);
  s.reach_mm = n["reach_mm"].as<double>(0.0);
  s.weight_kg = n["weight_kg"].as<double>(0.0);
  s.repeatability_mm = n["repeatability_mm"].as<double>(0.0);
  s.max_speed_ms = n["max_speed_ms"].as<double>(0.0);
  if (n["mounting"]) for (const auto& m : n["mounting"]) s.mounting_options.push_back(m.as<std::string>());
  s.safety_certified = n["safety_certified"].as<bool>(false);
  s.collaborative = n["collaborative"].as<bool>(false);
  s.torque_sensing = n["torque_sensing"].as<bool>(false);
  return s;
}

void RobotCatalogCore::loadFromFile(const std::string& path) {
  YAML::Node root;
  try {
    root = YAML::LoadFile(path);
  } catch (const std::exception& e) {
    throw std::runtime_error(std::string("catalog yaml load failed: ") + e.what());
  }
  robots_.clear();
  categories_.clear();
  if (root["categories"]) {
    for (const auto& kv : root["categories"]) {
      CategoryInfo c;
      c.id = kv.first.as<std::string>();
      const auto& n = kv.second;
      c.display_name = n["display_name"].as<std::string>("");
      c.description = n["description"].as<std::string>("");
      c.manufacturer = n["manufacturer"].as<std::string>("");
      c.website = n["website"].as<std::string>("");
      categories_[c.id] = c;
    }
  }
  if (root["robots"]) {
    for (const auto& kv : root["robots"]) {
      RobotConfig r;
      r.id = kv.first.as<std::string>();
      const auto& n = kv.second;
      r.display_name = n["display_name"].as<std::string>("");
      r.description = n["description"].as<std::string>("");
      r.image_path = n["image_path"].as<std::string>("");
      r.urdf_package = n["urdf_package"].as<std::string>("");
      r.urdf_path = n["urdf_path"].as<std::string>("");
      r.xacro_args = n["xacro_args"].as<std::string>("");
      r.category = n["category"].as<std::string>("");
      r.specifications = parseSpecs(n["specifications"]);
      if (n["required_packages"]) for (const auto& p : n["required_packages"]) r.required_packages.push_back(p.as<std::string>());
      if (n["optional_packages"]) for (const auto& p : n["optional_packages"]) r.optional_packages.push_back(p.as<std::string>());
      if (n["tags"]) for (const auto& t : n["tags"]) r.tags.push_back(t.as<std::string>());
      robots_[r.id] = r;
    }
  }
}

std::vector<RobotConfig> RobotCatalogCore::getAllRobots() const {
  std::vector<RobotConfig> out;
  out.reserve(robots_.size());
  for (const auto& kv : robots_) out.push_back(kv.second);
  return out;
}

bool RobotCatalogCore::getRobotById(const std::string& id, RobotConfig& out) const {
  auto it = robots_.find(id);
  if (it == robots_.end()) return false;
  out = it->second;
  return true;
}

std::vector<CategoryInfo> RobotCatalogCore::getCategories() const {
  std::vector<CategoryInfo> out;
  for (const auto& kv : categories_) out.push_back(kv.second);
  return out;
}

std::vector<RobotConfig> RobotCatalogCore::getRobotsByCategory(const std::string& category) const {
  std::vector<RobotConfig> out;
  for (const auto& kv : robots_) if (kv.second.category == category) out.push_back(kv.second);
  return out;
}

bool RobotCatalogCore::matchesFilter(const RobotConfig& r, const RobotFilter& f) {
  const auto& s = r.specifications;
  if (f.category && r.category != *f.category) return false;
  if (f.min_payload && s.payload_kg < *f.min_payload) return false;
  if (f.max_payload && s.payload_kg > *f.max_payload) return false;
  if (f.min_reach && s.reach_mm < *f.min_reach) return false;
  if (f.max_reach && s.reach_mm > *f.max_reach) return false;
  if (f.degrees_of_freedom && s.degrees_of_freedom != *f.degrees_of_freedom) return false;
  if (f.collaborative_only && *f.collaborative_only && !s.collaborative) return false;
  for (const auto& tag : f.required_tags)
    if (std::find(r.tags.begin(), r.tags.end(), tag) == r.tags.end()) return false;
  if (!f.search_text.empty() && !matchesSearch(r, f.search_text)) return false;
  return true;
}

bool RobotCatalogCore::matchesSearch(const RobotConfig& r, const std::string& term) {
  std::string needle = lower(term);
  std::vector<std::string> hay = {r.display_name, r.description, r.category};
  hay.insert(hay.end(), r.tags.begin(), r.tags.end());
  for (auto& h : hay) if (lower(h).find(needle) != std::string::npos) return true;
  return false;
}

std::vector<RobotConfig> RobotCatalogCore::filterRobots(const RobotFilter& f) const {
  if (f.isEmpty()) return getAllRobots();
  std::vector<RobotConfig> out;
  for (const auto& kv : robots_) if (matchesFilter(kv.second, f)) out.push_back(kv.second);
  return out;
}

std::vector<RobotConfig> RobotCatalogCore::searchRobots(const std::string& term) const {
  if (term.empty()) return getAllRobots();
  std::vector<RobotConfig> out;
  for (const auto& kv : robots_) if (matchesSearch(kv.second, term)) out.push_back(kv.second);
  return out;
}

std::vector<std::string> RobotCatalogCore::getMissingPackages(const RobotConfig& r) const {
  std::vector<std::string> missing;
  for (const auto& pkg : r.required_packages) {
    try {
      ament_index_cpp::get_package_share_directory(pkg);
    } catch (const std::exception&) {
      missing.push_back(pkg);
    }
  }
  return missing;
}

bool RobotCatalogCore::validateRobotPackages(const RobotConfig& r) const {
  return getMissingPackages(r).empty();
}

}  // namespace robot_catalog
```

- [ ] **Step 3: Commit**

```bash
git add robot_catalog_core/include/robot_catalog_core/catalog.hpp robot_catalog_core/src/catalog.cpp
git commit -m "feat(catalog-core): RobotCatalogCore load/filter/search/validate"
```

### Task 2.4: gtest parity suite + build

**Files:**
- Create: `robot_catalog_core/test/test_catalog.cpp`

- [ ] **Step 1: Write the gtest**

```cpp
#include <gtest/gtest.h>
#include <cstdlib>
#include "robot_catalog_core/catalog.hpp"
#include "robot_catalog_core/json.hpp"

using robot_catalog::RobotCatalogCore;
using robot_catalog::RobotFilter;
using robot_catalog::RobotConfig;

static std::string yaml_path() {
  // Set by the test command below to the repo's robots.yaml.
  const char* p = std::getenv("RDSA_ROBOTS_YAML");
  return p ? p : "";
}

class CatalogTest : public ::testing::Test {
protected:
  RobotCatalogCore cat;
  void SetUp() override { cat.loadFromFile(yaml_path()); }
};

TEST_F(CatalogTest, LoadsAllRobotsAndCategories) {
  EXPECT_EQ(cat.getAllRobots().size(), 20u);
  EXPECT_EQ(cat.getCategories().size(), 3u);
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
  EXPECT_EQ(cat.filterRobots(RobotFilter{}).size(), 20u);
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
```

- [ ] **Step 2: Build + run the test**

```bash
cd /home/bot/rds_ws
source /opt/ros/humble/setup.bash
colcon build --merge-install --packages-select robot_catalog_core
# gtest needs the yaml path; point it at the repo file:
export RDSA_ROBOTS_YAML="$(pwd)/src/robot-description-setup-assistant/robot_description_setup_assistant/config/robots.yaml"
colcon test --merge-install --packages-select robot_catalog_core --ctest-args -V 2>&1 | tail -20
colcon test-result --verbose --test-result-base build/robot_catalog_core | tail -5
```
Expected: `LoadsAllRobotsAndCategories`, `Ur3Fields`, `FilterByCategory`, `EmptyFilterReturnsAll`, `SearchCaseInsensitive`, `JsonRoundtripShape` all pass.

Note: if `colcon test` does not propagate the env to ctest, run the test binary directly:
`RDSA_ROBOTS_YAML=... ./build/robot_catalog_core/test_catalog`.

- [ ] **Step 3: Commit**

```bash
cd /home/bot/rds_ws/src/robot-description-setup-assistant
git add robot_catalog_core/test/test_catalog.cpp
git commit -m "test(catalog-core): gtest parity suite (20 robots, filter, search)"
```

---

## PHASE 3 — `robot_catalog_msgs` + `robot_catalog_server`

### Task 3.1: Service definitions package

**Files:**
- Create: `robot_catalog_msgs/package.xml`, `CMakeLists.txt`, `srv/*.srv`

- [ ] **Step 1: `package.xml`**

```xml
<?xml version="1.0"?>
<package format="3">
  <name>robot_catalog_msgs</name>
  <version>0.1.0</version>
  <description>Service definitions for the robot catalog server.</description>
  <maintainer email="sshaizkhan@gmail.com">Shahwaz Khan</maintainer>
  <license>BSD</license>
  <buildtool_depend>ament_cmake</buildtool_depend>
  <buildtool_depend>rosidl_default_generators</buildtool_depend>
  <exec_depend>rosidl_default_runtime</exec_depend>
  <member_of_group>rosidl_interface_packages</member_of_group>
  <export><build_type>ament_cmake</build_type></export>
</package>
```

- [ ] **Step 2: `srv/GetRobots.srv`, `srv/GetCategories.srv`**

`srv/GetRobots.srv`:
```
---
string robots_json
```
`srv/GetCategories.srv`:
```
---
string categories_json
```

- [ ] **Step 3: `srv/FilterRobots.srv` (typed input, JSON output)**

```
string category
bool has_min_payload
float64 min_payload
bool has_max_payload
float64 max_payload
bool has_min_reach
float64 min_reach
bool has_max_reach
float64 max_reach
bool has_dof
int32 degrees_of_freedom
bool has_collaborative_only
bool collaborative_only
string[] required_tags
string search_text
---
string robots_json
```

- [ ] **Step 4: `srv/ValidateRobot.srv`**

```
string robot_id
---
bool found
bool ok
string[] missing_packages
```

- [ ] **Step 5: `CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.10)
project(robot_catalog_msgs)
find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
rosidl_generate_interfaces(${PROJECT_NAME}
  "srv/GetRobots.srv"
  "srv/GetCategories.srv"
  "srv/FilterRobots.srv"
  "srv/ValidateRobot.srv"
)
ament_export_dependencies(rosidl_default_runtime)
ament_package()
```

- [ ] **Step 6: Build the messages**

```bash
cd /home/bot/rds_ws && source /opt/ros/humble/setup.bash
colcon build --merge-install --packages-select robot_catalog_msgs 2>&1 | tail -3
```
Expected: `Finished <<< robot_catalog_msgs`.

- [ ] **Step 7: Commit**

```bash
cd /home/bot/rds_ws/src/robot-description-setup-assistant
git add robot_catalog_msgs
git commit -m "feat(catalog-msgs): GetRobots/GetCategories/FilterRobots/ValidateRobot srv"
```

### Task 3.2: `robot_catalog_server` node

**Files:**
- Create: `robot_catalog_server/package.xml`, `CMakeLists.txt`, `src/server_node.cpp`

- [ ] **Step 1: `package.xml`**

```xml
<?xml version="1.0"?>
<package format="3">
  <name>robot_catalog_server</name>
  <version>0.1.0</version>
  <description>rclcpp node exposing the robot catalog over services.</description>
  <maintainer email="sshaizkhan@gmail.com">Shahwaz Khan</maintainer>
  <license>BSD</license>
  <buildtool_depend>ament_cmake</buildtool_depend>
  <depend>rclcpp</depend>
  <depend>robot_catalog_core</depend>
  <depend>robot_catalog_msgs</depend>
  <depend>ament_index_cpp</depend>
  <export><build_type>ament_cmake</build_type></export>
</package>
```

- [ ] **Step 2: `src/server_node.cpp`**

```cpp
#include <rclcpp/rclcpp.hpp>
#include <ament_index_cpp/get_package_share_directory.hpp>
#include "robot_catalog_core/catalog.hpp"
#include "robot_catalog_core/json.hpp"
#include "robot_catalog_msgs/srv/get_robots.hpp"
#include "robot_catalog_msgs/srv/get_categories.hpp"
#include "robot_catalog_msgs/srv/filter_robots.hpp"
#include "robot_catalog_msgs/srv/validate_robot.hpp"

using robot_catalog::RobotCatalogCore;
using robot_catalog::RobotFilter;

class CatalogServer : public rclcpp::Node {
public:
  CatalogServer() : Node("robot_catalog_server") {
    std::string path = this->declare_parameter<std::string>("robots_yaml", default_yaml());
    core_.loadFromFile(path);
    RCLCPP_INFO(get_logger(), "loaded catalog: %zu robots", core_.getAllRobots().size());

    get_robots_ = create_service<robot_catalog_msgs::srv::GetRobots>(
      "catalog/get_robots",
      [this](const std::shared_ptr<robot_catalog_msgs::srv::GetRobots::Request>,
             std::shared_ptr<robot_catalog_msgs::srv::GetRobots::Response> res) {
        res->robots_json = robot_catalog::robots_to_json(core_.getAllRobots());
      });

    get_categories_ = create_service<robot_catalog_msgs::srv::GetCategories>(
      "catalog/get_categories",
      [this](const std::shared_ptr<robot_catalog_msgs::srv::GetCategories::Request>,
             std::shared_ptr<robot_catalog_msgs::srv::GetCategories::Response> res) {
        res->categories_json = robot_catalog::categories_to_json(core_.getCategories());
      });

    filter_ = create_service<robot_catalog_msgs::srv::FilterRobots>(
      "catalog/filter_robots",
      [this](const std::shared_ptr<robot_catalog_msgs::srv::FilterRobots::Request> req,
             std::shared_ptr<robot_catalog_msgs::srv::FilterRobots::Response> res) {
        RobotFilter f;
        if (!req->category.empty()) f.category = req->category;
        if (req->has_min_payload) f.min_payload = req->min_payload;
        if (req->has_max_payload) f.max_payload = req->max_payload;
        if (req->has_min_reach) f.min_reach = req->min_reach;
        if (req->has_max_reach) f.max_reach = req->max_reach;
        if (req->has_dof) f.degrees_of_freedom = req->degrees_of_freedom;
        if (req->has_collaborative_only) f.collaborative_only = req->collaborative_only;
        f.required_tags = req->required_tags;
        f.search_text = req->search_text;
        res->robots_json = robot_catalog::robots_to_json(core_.filterRobots(f));
      });

    validate_ = create_service<robot_catalog_msgs::srv::ValidateRobot>(
      "catalog/validate_robot",
      [this](const std::shared_ptr<robot_catalog_msgs::srv::ValidateRobot::Request> req,
             std::shared_ptr<robot_catalog_msgs::srv::ValidateRobot::Response> res) {
        robot_catalog::RobotConfig r;
        if (!core_.getRobotById(req->robot_id, r)) { res->found = false; res->ok = false; return; }
        res->found = true;
        res->missing_packages = core_.getMissingPackages(r);
        res->ok = res->missing_packages.empty();
      });
  }

private:
  static std::string default_yaml() {
    return ament_index_cpp::get_package_share_directory("robot_description_setup_assistant") +
           "/config/robots.yaml";
  }
  RobotCatalogCore core_;
  rclcpp::Service<robot_catalog_msgs::srv::GetRobots>::SharedPtr get_robots_;
  rclcpp::Service<robot_catalog_msgs::srv::GetCategories>::SharedPtr get_categories_;
  rclcpp::Service<robot_catalog_msgs::srv::FilterRobots>::SharedPtr filter_;
  rclcpp::Service<robot_catalog_msgs::srv::ValidateRobot>::SharedPtr validate_;
};

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<CatalogServer>());
  rclcpp::shutdown();
  return 0;
}
```

- [ ] **Step 3: `CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.10)
project(robot_catalog_server)
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(robot_catalog_core REQUIRED)
find_package(robot_catalog_msgs REQUIRED)
find_package(ament_index_cpp REQUIRED)

add_executable(robot_catalog_server src/server_node.cpp)
ament_target_dependencies(robot_catalog_server rclcpp robot_catalog_core robot_catalog_msgs ament_index_cpp)
install(TARGETS robot_catalog_server DESTINATION lib/${PROJECT_NAME})
ament_package()
```

- [ ] **Step 4: Build + smoke test the services**

```bash
cd /home/bot/rds_ws && source /opt/ros/humble/setup.bash
colcon build --merge-install --packages-select robot_catalog_core robot_catalog_msgs robot_catalog_server 2>&1 | tail -4
source install/setup.bash
ros2 run robot_catalog_server robot_catalog_server &
SRV=$!; sleep 3
ros2 service list | grep catalog
ros2 service call /catalog/get_categories robot_catalog_msgs/srv/GetCategories | head -c 200; echo
ros2 service call /catalog/filter_robots robot_catalog_msgs/srv/FilterRobots "{category: 'universal_robots'}" 2>&1 | grep -o 'ur3' | head -1
kill $SRV
```
Expected: services listed; categories JSON returned; filter returns JSON containing `ur3`.

- [ ] **Step 5: Commit**

```bash
cd /home/bot/rds_ws/src/robot-description-setup-assistant
git add robot_catalog_server
git commit -m "feat(catalog-server): rclcpp node exposing catalog services"
```

---

## PHASE 4 — FastAPI rclpy bridge

### Task 4.1: `catalog_client.py` (rclpy service client)

**Files:**
- Create: `web/backend/rdsa_web/catalog_client.py`
- Test: `web/backend/tests/test_catalog_client.py`

- [ ] **Step 1: Write the failing test (pure mapping helpers — no ROS)**

`web/backend/tests/test_catalog_client.py`:
```python
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
    robots = parse_robots_json('[{"id":"ur3","display_name":"UR3","category":"universal_robots"}]')
    assert robots[0].id == "ur3"
    assert robots[0].display_name == "UR3"
```

- [ ] **Step 2: Run test to verify it fails**

`cd web/backend && . .venv/bin/activate && pytest tests/test_catalog_client.py -v` → ModuleNotFoundError.

- [ ] **Step 3: Write `catalog_client.py`**

```python
"""Bridge: call the C++ robot_catalog_server over ROS services via rclpy."""
from __future__ import annotations

import json
import threading

from .models import CategoryInfo, RobotConfig, RobotFilter


def filter_to_request_fields(f: RobotFilter) -> dict:
    return {
        "category": f.category or "",
        "has_min_payload": f.min_payload is not None,
        "min_payload": f.min_payload or 0.0,
        "has_max_payload": f.max_payload is not None,
        "max_payload": f.max_payload or 0.0,
        "has_min_reach": f.min_reach is not None,
        "min_reach": f.min_reach or 0.0,
        "has_max_reach": f.max_reach is not None,
        "max_reach": f.max_reach or 0.0,
        "has_dof": f.degrees_of_freedom is not None,
        "degrees_of_freedom": f.degrees_of_freedom or 0,
        "has_collaborative_only": f.collaborative_only is not None,
        "collaborative_only": bool(f.collaborative_only),
        "required_tags": list(f.required_tags),
        "search_text": f.search_text or "",
    }


def parse_robots_json(text: str) -> list[RobotConfig]:
    return [RobotConfig(**r) for r in json.loads(text)]


def parse_categories_json(text: str) -> list[CategoryInfo]:
    return [CategoryInfo(**c) for c in json.loads(text)]


class CatalogServiceUnavailable(Exception):
    pass


class CatalogClient:
    """Thin rclpy client. Spins its own node; one call at a time (lock)."""

    def __init__(self, timeout_sec: float = 5.0) -> None:
        import rclpy
        from robot_catalog_msgs.srv import (
            FilterRobots,
            GetCategories,
            GetRobots,
            ValidateRobot,
        )

        if not rclpy.ok():
            rclpy.init()
        self._rclpy = rclpy
        self._node = rclpy.create_node("rdsa_catalog_client")
        self._timeout = timeout_sec
        self._lock = threading.Lock()
        self._get_robots = self._node.create_client(GetRobots, "catalog/get_robots")
        self._get_categories = self._node.create_client(GetCategories, "catalog/get_categories")
        self._filter = self._node.create_client(FilterRobots, "catalog/filter_robots")
        self._validate = self._node.create_client(ValidateRobot, "catalog/validate_robot")
        self._types = {
            "GetRobots": GetRobots,
            "GetCategories": GetCategories,
            "FilterRobots": FilterRobots,
            "ValidateRobot": ValidateRobot,
        }

    def _call(self, client, request):
        with self._lock:
            if not client.wait_for_service(timeout_sec=self._timeout):
                raise CatalogServiceUnavailable("robot_catalog_server not available")
            future = client.call_async(request)
            self._rclpy.spin_until_future_complete(
                self._node, future, timeout_sec=self._timeout
            )
            if not future.done() or future.result() is None:
                raise CatalogServiceUnavailable("catalog service call timed out")
            return future.result()

    def get_all_robots(self) -> list[RobotConfig]:
        res = self._call(self._get_robots, self._types["GetRobots"].Request())
        return parse_robots_json(res.robots_json)

    def get_categories(self) -> list[CategoryInfo]:
        res = self._call(self._get_categories, self._types["GetCategories"].Request())
        return parse_categories_json(res.categories_json)

    def filter_robots(self, flt: RobotFilter) -> list[RobotConfig]:
        req = self._types["FilterRobots"].Request()
        for k, v in filter_to_request_fields(flt).items():
            setattr(req, k, v)
        res = self._call(self._filter, req)
        return parse_robots_json(res.robots_json)

    def validate(self, robot_id: str) -> dict:
        req = self._types["ValidateRobot"].Request()
        req.robot_id = robot_id
        res = self._call(self._validate, req)
        return {"ok": bool(res.ok), "missing_packages": list(res.missing_packages)}
```

- [ ] **Step 4: Run test to verify it passes**

`cd web/backend && . .venv/bin/activate && pytest tests/test_catalog_client.py -v` → 2 passed.
(The pure helpers don't import rclpy; `CatalogClient.__init__` does, only when constructed.)

- [ ] **Step 5: Commit**

```bash
git add web/backend/rdsa_web/catalog_client.py web/backend/tests/test_catalog_client.py
git commit -m "feat(web): rclpy catalog service client + pure mappers"
```

### Task 4.2: Wire the bridge into the API (behind a flag, default on)

**Files:**
- Modify: `web/backend/rdsa_web/app.py`
- Modify: `web/backend/tests/test_api.py`

- [ ] **Step 1: Add a catalog-source abstraction in `create_app`**

The endpoints currently call a `RobotCatalog` (YAML). Introduce a `catalog`
object that is either the YAML `RobotCatalog` (default, for tests) or the
`CatalogClient` (when `RDSA_CATALOG_BACKEND=cpp`). Both expose
`get_all_robots()`, `get_categories()`, `filter_robots(f)`. Validation uses the
client's `validate(id)` when in cpp mode, else `get_missing_packages`.

In `app.py`, change `create_app` to accept an optional `catalog` and pick the
source:

```python
def _make_catalog():
    import os
    if os.environ.get("RDSA_CATALOG_BACKEND") == "cpp":
        from .catalog_client import CatalogClient
        return CatalogClient()
    return _default_catalog()  # existing YAML RobotCatalog
```

and use `catalog = catalog or _make_catalog()`. Wrap the catalog/filter/validate
handlers so a `CatalogServiceUnavailable` becomes `HTTPException(503)`:

```python
from .catalog_client import CatalogServiceUnavailable

def _guard(fn):
    try:
        return fn()
    except CatalogServiceUnavailable as exc:
        raise HTTPException(status_code=503, detail=str(exc))
```

Use it in `robots()`, `categories()`, `filter_robots()`. For `/validate` in cpp
mode call `catalog.validate(robot_id)`; otherwise keep `get_missing_packages`.
Keep the existing YAML behavior as the default so all current tests pass
unchanged.

- [ ] **Step 2: Add a test that 503 is returned when the client is unavailable**

Append to `web/backend/tests/test_api.py`:
```python
def test_catalog_503_when_service_unavailable():
    from rdsa_web.app import create_app
    from rdsa_web.catalog_client import CatalogServiceUnavailable

    class DownClient:
        def get_all_robots(self):
            raise CatalogServiceUnavailable("down")
        def get_categories(self):
            raise CatalogServiceUnavailable("down")
        def filter_robots(self, f):
            raise CatalogServiceUnavailable("down")

    app = create_app(catalog=DownClient())
    client = TestClient(app)
    assert client.get("/api/robots").status_code == 503
```

- [ ] **Step 3: Run tests**

`cd web/backend && . .venv/bin/activate && pytest tests/ -q` → all pass (default YAML path unchanged; new 503 test passes).

- [ ] **Step 4: Commit**

```bash
git add web/backend/rdsa_web/app.py web/backend/tests/test_api.py
git commit -m "feat(web): route catalog through C++ service when RDSA_CATALOG_BACKEND=cpp; 503 on outage"
```

### Task 4.3: Launch wiring + end-to-end smoke

**Files:**
- Modify: `robot_description_setup_assistant/launch/webui.launch.py`

- [ ] **Step 1: Start the C++ node in the launch file**

Add a `Node(package="robot_catalog_server", executable="robot_catalog_server")`
to the `LaunchDescription`, and set `RDSA_CATALOG_BACKEND=cpp` in the uvicorn
`ExecuteProcess` `additional_env`.

- [ ] **Step 2: End-to-end smoke (manual)**

```bash
cd /home/bot/rds_ws && source /opt/ros/humble/setup.bash && source install/setup.bash
ros2 run robot_catalog_server robot_catalog_server &   # C++ catalog
SRV=$!
cd src/robot-description-setup-assistant/web/backend && . .venv/bin/activate
RDSA_CATALOG_BACKEND=cpp \
RDSA_ROBOTS_YAML="$(cd ../.. && pwd)/robot_description_setup_assistant/config/robots.yaml" \
  uvicorn rdsa_web.app:app --port 8000 &
UV=$!; sleep 3
curl -s localhost:8000/api/robots | python3 -c "import sys,json;print('robots:',len(json.load(sys.stdin)))"
curl -s localhost:8000/api/categories | python3 -c "import sys,json;print('cats:',len(json.load(sys.stdin)))"
curl -s -X POST localhost:8000/api/robots/filter -H 'content-type: application/json' -d '{"category":"universal_robots"}' | python3 -c "import sys,json;print('ur:',len(json.load(sys.stdin)))"
kill $UV $SRV
```
Expected: `robots: 20`, `cats: 3`, `ur: 9` — now served by the **C++** node.

- [ ] **Step 3: Commit**

```bash
cd /home/bot/rds_ws/src/robot-description-setup-assistant
git add robot_description_setup_assistant/launch/webui.launch.py
git commit -m "feat: launch starts C++ catalog node; backend uses it"
```

---

## Definition of done

- `colcon test --packages-select robot_catalog_core` — gtest parity green (20 robots, 3 categories, filter=9, search, json).
- `robot_catalog_server` advertises the four `catalog/*` services; `ros2 service call` returns correct JSON.
- Backend test suite green (YAML default path unchanged; 503 test passes).
- E2E: with the C++ node running and `RDSA_CATALOG_BACKEND=cpp`, `/api/robots`=20, `/api/categories`=3, filter `universal_robots`=9 — served by C++.
- React frontend + HTTP contract unchanged.
- `feat/webui-rewrite` untouched.

## Notes for the executor

- Build order matters: `robot_catalog_core` → `robot_catalog_msgs` → `robot_catalog_server`. Source `install/setup.bash` before running the node or the rclpy client (the generated `robot_catalog_msgs` Python module must be importable).
- The Python backend keeps the YAML path as default so CI/tests don't need ROS; the C++ path is opt-in via `RDSA_CATALOG_BACKEND=cpp`.
- Filter/search semantics in `catalog.cpp` must match `web/backend/rdsa_web/catalog.py` exactly (same parity tests on both sides).
- yaml-cpp `.as<T>(default)` is used for missing keys (matches the Python `.get(key, default)` behavior).
