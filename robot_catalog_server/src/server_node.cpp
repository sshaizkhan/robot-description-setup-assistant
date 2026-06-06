#include <rclcpp/rclcpp.hpp>
#include <algorithm>
#include <filesystem>
#include <vector>
#include <ament_index_cpp/get_package_share_directory.hpp>
#include "robot_catalog_core/catalog.hpp"
#include "robot_catalog_core/json.hpp"
#include "robot_catalog_core/urdf.hpp"
#include "robot_catalog_core/mesh.hpp"
#include "robot_catalog_msgs/srv/get_robots.hpp"
#include "robot_catalog_msgs/srv/get_categories.hpp"
#include "robot_catalog_msgs/srv/filter_robots.hpp"
#include "robot_catalog_msgs/srv/validate_robot.hpp"
#include "robot_catalog_msgs/srv/get_urdf.hpp"
#include "robot_catalog_msgs/srv/resolve_mesh.hpp"

using robot_catalog::RobotCatalogCore;
using robot_catalog::RobotFilter;

class CatalogServer : public rclcpp::Node {
public:
  CatalogServer() : Node("robot_catalog_server") {
    std::string path = this->declare_parameter<std::string>("catalog_path", default_catalog());
    core_.load(path);  // single YAML file or a directory of them
    RCLCPP_INFO(get_logger(), "loaded catalog from %s: %zu robots",
                path.c_str(), core_.getAllRobots().size());

    // Reentrant group: with the MultiThreadedExecutor, this lets multiple
    // service requests (e.g. all of a robot's mesh fetches) run concurrently
    // instead of one-at-a-time.
    cbg_ = create_callback_group(rclcpp::CallbackGroupType::Reentrant);

    get_robots_ = create_service<robot_catalog_msgs::srv::GetRobots>(
      "catalog/get_robots",
      [this](const std::shared_ptr<robot_catalog_msgs::srv::GetRobots::Request> req,
             std::shared_ptr<robot_catalog_msgs::srv::GetRobots::Response> res) {
        int total = 0;
        if (req->offset == 0 && req->limit <= 0) {
          // Whole list: serve the pre-built JSON cache.
          res->robots_json = core_.allRobotsJson();
          res->total = static_cast<int>(core_.getAllRobots().size());
        } else {
          res->robots_json =
              robot_catalog::robots_to_json(core_.getRobotsPage(req->offset, req->limit, total));
          res->total = total;
        }
      }, rmw_qos_profile_services_default, cbg_);

    get_categories_ = create_service<robot_catalog_msgs::srv::GetCategories>(
      "catalog/get_categories",
      [this](const std::shared_ptr<robot_catalog_msgs::srv::GetCategories::Request>,
             std::shared_ptr<robot_catalog_msgs::srv::GetCategories::Response> res) {
        res->categories_json = core_.categoriesJson();
      }, rmw_qos_profile_services_default, cbg_);

    filter_ = create_service<robot_catalog_msgs::srv::FilterRobots>(
      "catalog/filter_robots",
      [this](const std::shared_ptr<robot_catalog_msgs::srv::FilterRobots::Request> req,
             std::shared_ptr<robot_catalog_msgs::srv::FilterRobots::Response> res) {
        RobotFilter f;
        if (!req->type.empty()) f.type = req->type;
        if (!req->category.empty()) f.category = req->category;
        if (req->has_min_payload) f.min_payload = req->min_payload;
        if (req->has_max_payload) f.max_payload = req->max_payload;
        if (req->has_min_reach) f.min_reach = req->min_reach;
        if (req->has_max_reach) f.max_reach = req->max_reach;
        if (req->has_dof) f.degrees_of_freedom = req->degrees_of_freedom;
        if (req->has_collaborative_only) f.collaborative_only = req->collaborative_only;
        f.required_tags = req->required_tags;
        f.search_text = req->search_text;
        auto matched = core_.filterRobots(f);
        res->total = static_cast<int>(matched.size());
        int off = req->offset < 0 ? 0 : req->offset;
        int total = res->total;
        std::vector<robot_catalog::RobotConfig> page;
        if (off < total) {
          int end = (req->limit <= 0) ? total : std::min(total, off + req->limit);
          page.assign(matched.begin() + off, matched.begin() + end);
        }
        res->robots_json = robot_catalog::robots_to_json(page);
      }, rmw_qos_profile_services_default, cbg_);

    validate_ = create_service<robot_catalog_msgs::srv::ValidateRobot>(
      "catalog/validate_robot",
      [this](const std::shared_ptr<robot_catalog_msgs::srv::ValidateRobot::Request> req,
             std::shared_ptr<robot_catalog_msgs::srv::ValidateRobot::Response> res) {
        robot_catalog::RobotConfig r;
        if (!core_.getRobotById(req->robot_id, r)) {
          res->found = false;
          res->ok = false;
          return;
        }
        res->found = true;
        res->missing_packages = core_.getMissingPackages(r);
        res->ok = res->missing_packages.empty();
      }, rmw_qos_profile_services_default, cbg_);

    get_urdf_ = create_service<robot_catalog_msgs::srv::GetUrdf>(
      "catalog/get_urdf",
      [this](const std::shared_ptr<robot_catalog_msgs::srv::GetUrdf::Request> req,
             std::shared_ptr<robot_catalog_msgs::srv::GetUrdf::Response> res) {
        robot_catalog::RobotConfig r;
        if (!core_.getRobotById(req->robot_id, r)) {
          res->found = false;
          res->ok = false;
          return;
        }
        res->found = true;
        robot_catalog::UrdfResult u = robot_catalog::resolveUrdf(r);
        res->ok = u.ok;
        res->urdf_xml = u.xml;
        res->error = u.error;
        res->missing_packages = core_.getMissingPackages(r);
      }, rmw_qos_profile_services_default, cbg_);

    resolve_mesh_ = create_service<robot_catalog_msgs::srv::ResolveMesh>(
      "catalog/resolve_mesh",
      [this](const std::shared_ptr<robot_catalog_msgs::srv::ResolveMesh::Request> req,
             std::shared_ptr<robot_catalog_msgs::srv::ResolveMesh::Response> res) {
        robot_catalog::MeshResult m =
            robot_catalog::readMesh(req->package, req->rel_path);
        res->ok = m.ok;
        res->too_large = m.too_large;
        res->size_bytes = m.size;
        res->data = m.data;
        res->media_type = m.media_type;
      }, rmw_qos_profile_services_default, cbg_);
  }

private:
  // Prefer the multi-file catalog directory; fall back to the legacy single file.
  static std::string default_catalog() {
    std::string share =
        ament_index_cpp::get_package_share_directory("robot_description_setup_assistant");
    std::string dir = share + "/config/catalog";
    std::error_code ec;
    if (std::filesystem::is_directory(dir, ec)) return dir;
    return share + "/config/robots.yaml";
  }
  RobotCatalogCore core_;
  rclcpp::CallbackGroup::SharedPtr cbg_;
  rclcpp::Service<robot_catalog_msgs::srv::GetRobots>::SharedPtr get_robots_;
  rclcpp::Service<robot_catalog_msgs::srv::GetCategories>::SharedPtr get_categories_;
  rclcpp::Service<robot_catalog_msgs::srv::FilterRobots>::SharedPtr filter_;
  rclcpp::Service<robot_catalog_msgs::srv::ValidateRobot>::SharedPtr validate_;
  rclcpp::Service<robot_catalog_msgs::srv::GetUrdf>::SharedPtr get_urdf_;
  rclcpp::Service<robot_catalog_msgs::srv::ResolveMesh>::SharedPtr resolve_mesh_;
};

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  // Multi-threaded so concurrent mesh/URDF requests are served in parallel
  // (the web layer fires all of a robot's mesh requests at once). The catalog
  // data is loaded once and only read thereafter, so concurrent reads are safe.
  auto node = std::make_shared<CatalogServer>();
  rclcpp::executors::MultiThreadedExecutor executor;
  executor.add_node(node);
  executor.spin();
  rclcpp::shutdown();
  return 0;
}
