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
  if (n["mounting"])
    for (const auto& m : n["mounting"]) s.mounting_options.push_back(m.as<std::string>());
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
      if (n["required_packages"])
        for (const auto& p : n["required_packages"]) r.required_packages.push_back(p.as<std::string>());
      if (n["optional_packages"])
        for (const auto& p : n["optional_packages"]) r.optional_packages.push_back(p.as<std::string>());
      if (n["tags"])
        for (const auto& t : n["tags"]) r.tags.push_back(t.as<std::string>());
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
  for (const auto& kv : robots_)
    if (kv.second.category == category) out.push_back(kv.second);
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
  for (auto& h : hay)
    if (lower(h).find(needle) != std::string::npos) return true;
  return false;
}

std::vector<RobotConfig> RobotCatalogCore::filterRobots(const RobotFilter& f) const {
  if (f.isEmpty()) return getAllRobots();
  std::vector<RobotConfig> out;
  for (const auto& kv : robots_)
    if (matchesFilter(kv.second, f)) out.push_back(kv.second);
  return out;
}

std::vector<RobotConfig> RobotCatalogCore::searchRobots(const std::string& term) const {
  if (term.empty()) return getAllRobots();
  std::vector<RobotConfig> out;
  for (const auto& kv : robots_)
    if (matchesSearch(kv.second, term)) out.push_back(kv.second);
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
