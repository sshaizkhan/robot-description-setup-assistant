#include "robot_catalog_core/catalog.hpp"
#include "robot_catalog_core/json.hpp"
#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <yaml-cpp/yaml.h>
#include <ament_index_cpp/get_package_share_directory.hpp>

namespace fs = std::filesystem;

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
  if (n["mounting"])
    for (const auto& m : n["mounting"]) s.mounting_options.push_back(m.as<std::string>());
  s.safety_certified = n["safety_certified"].as<bool>(false);
  s.collaborative = n["collaborative"].as<bool>(false);
  s.torque_sensing = n["torque_sensing"].as<bool>(false);
  return s;
}

static AttachInfo parseAttach(const YAML::Node& n) {
  AttachInfo a;
  if (!n) return a;
  a.tool_frame = n["tool_frame"].as<std::string>("");
  a.mount_frame = n["mount_frame"].as<std::string>("");
  if (n["xyz"] && n["xyz"].IsSequence())
    for (size_t i = 0; i < 3 && i < n["xyz"].size(); ++i) a.xyz[i] = n["xyz"][i].as<double>(0.0);
  if (n["rpy"] && n["rpy"].IsSequence())
    for (size_t i = 0; i < 3 && i < n["rpy"].size(); ++i) a.rpy[i] = n["rpy"][i].as<double>(0.0);
  return a;
}

static CategoryInfo parseCategory(const std::string& id, const YAML::Node& n) {
  CategoryInfo c;
  c.id = id;
  c.display_name = n["display_name"].as<std::string>("");
  c.description = n["description"].as<std::string>("");
  c.manufacturer = n["manufacturer"].as<std::string>("");
  c.website = n["website"].as<std::string>("");
  return c;
}

static RobotConfig parseRobot(const std::string& id, const YAML::Node& n) {
  RobotConfig r;
  r.id = id;
  r.display_name = n["display_name"].as<std::string>("");
  r.description = n["description"].as<std::string>("");
  r.image_path = n["image_path"].as<std::string>("");
  r.urdf_package = n["urdf_package"].as<std::string>("");
  r.urdf_path = n["urdf_path"].as<std::string>("");
  r.xacro_args = n["xacro_args"].as<std::string>("");
  r.category = n["category"].as<std::string>("");
  r.type = n["type"].as<std::string>("arm");
  r.attach = parseAttach(n["attach"]);
  r.specifications = parseSpecs(n["specifications"]);
  if (n["required_packages"])
    for (const auto& p : n["required_packages"]) r.required_packages.push_back(p.as<std::string>());
  if (n["optional_packages"])
    for (const auto& p : n["optional_packages"]) r.optional_packages.push_back(p.as<std::string>());
  if (n["tags"])
    for (const auto& t : n["tags"]) r.tags.push_back(t.as<std::string>());
  return r;
}

void RobotCatalogCore::mergeFile(const std::string& path) {
  YAML::Node root;
  try {
    root = YAML::LoadFile(path);
  } catch (const std::exception& e) {
    throw std::runtime_error("catalog yaml load failed (" + path + "): " + e.what());
  }
  if (root["categories"]) {
    for (const auto& kv : root["categories"]) {
      std::string id = kv.first.as<std::string>();
      categories_[id] = parseCategory(id, kv.second);
    }
  }
  if (root["robots"]) {
    for (const auto& kv : root["robots"]) {
      std::string id = kv.first.as<std::string>();
      robots_[id] = parseRobot(id, kv.second);
    }
  }
}

void RobotCatalogCore::load(const std::string& path) {
  std::error_code ec;
  if (fs::is_directory(path, ec)) {
    loadFromDirectory(path);
  } else {
    loadFromFile(path);
  }
}

void RobotCatalogCore::loadFromFile(const std::string& path) {
  robots_.clear();
  categories_.clear();
  mergeFile(path);
  rebuildCaches();
}

void RobotCatalogCore::loadFromDirectory(const std::string& dir) {
  robots_.clear();
  categories_.clear();
  std::vector<fs::path> files;
  for (const auto& entry : fs::recursive_directory_iterator(dir)) {
    if (!entry.is_regular_file()) continue;
    auto ext = entry.path().extension().string();
    if (ext == ".yml" || ext == ".yaml") files.push_back(entry.path());
  }
  // Deterministic merge order so later files override earlier ones predictably.
  std::sort(files.begin(), files.end());
  for (const auto& f : files) mergeFile(f.string());
  rebuildCaches();
}

void RobotCatalogCore::rebuildCaches() {
  sorted_robots_.clear();
  sorted_robots_.reserve(robots_.size());
  for (const auto& kv : robots_) sorted_robots_.push_back(kv.second);
  std::sort(sorted_robots_.begin(), sorted_robots_.end(),
            [](const RobotConfig& a, const RobotConfig& b) { return a.id < b.id; });
  all_robots_json_ = robots_to_json(sorted_robots_);
  categories_json_ = categories_to_json(getCategories());
}

std::vector<RobotConfig> RobotCatalogCore::getAllRobots() const {
  return sorted_robots_;
}

std::vector<RobotConfig> RobotCatalogCore::getRobotsPage(int offset, int limit,
                                                         int& total) const {
  total = static_cast<int>(sorted_robots_.size());
  std::vector<RobotConfig> out;
  if (offset < 0) offset = 0;
  if (offset >= total) return out;
  int end = (limit <= 0) ? total : std::min(total, offset + limit);
  for (int i = offset; i < end; ++i) out.push_back(sorted_robots_[i]);
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
  std::sort(out.begin(), out.end(),
            [](const CategoryInfo& a, const CategoryInfo& b) { return a.id < b.id; });
  return out;
}

std::vector<RobotConfig> RobotCatalogCore::getRobotsByCategory(const std::string& category) const {
  std::vector<RobotConfig> out;
  for (const auto& r : sorted_robots_)
    if (r.category == category) out.push_back(r);
  return out;
}

bool RobotCatalogCore::matchesFilter(const RobotConfig& r, const RobotFilter& f) {
  const auto& s = r.specifications;
  if (f.type && r.type != *f.type) return false;
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
  for (auto& h : hay)
    if (lower(h).find(needle) != std::string::npos) return true;
  return false;
}

std::vector<RobotConfig> RobotCatalogCore::filterRobots(const RobotFilter& f) const {
  if (f.isEmpty()) return getAllRobots();
  std::vector<RobotConfig> out;
  for (const auto& r : sorted_robots_)
    if (matchesFilter(r, f)) out.push_back(r);
  return out;
}

std::vector<RobotConfig> RobotCatalogCore::searchRobots(const std::string& term) const {
  if (term.empty()) return getAllRobots();
  std::vector<RobotConfig> out;
  for (const auto& r : sorted_robots_)
    if (matchesSearch(r, term)) out.push_back(r);
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
