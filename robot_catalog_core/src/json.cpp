#include "robot_catalog_core/json.hpp"
#include <cstdio>
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

static std::string num_array(const double* v, size_t n) {
  std::string out = "[";
  for (size_t i = 0; i < n; ++i) {
    if (i) out += ",";
    out += num(v[i]);
  }
  return out + "]";
}

static std::string attach_json(const AttachInfo& a) {
  std::string out = "{";
  out += "\"tool_frame\":" + q(a.tool_frame);
  out += ",\"mount_frame\":" + q(a.mount_frame);
  out += ",\"xyz\":" + num_array(a.xyz, 3);
  out += ",\"rpy\":" + num_array(a.rpy, 3);
  return out + "}";
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
  out += ",\"type\":" + q(r.type);
  out += ",\"attach\":" + attach_json(r.attach);
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
