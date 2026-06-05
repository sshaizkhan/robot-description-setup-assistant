#include <rclcpp/rclcpp.hpp>
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
        if (!core_.getRobotById(req->robot_id, r)) {
          res->found = false;
          res->ok = false;
          return;
        }
        res->found = true;
        res->missing_packages = core_.getMissingPackages(r);
        res->ok = res->missing_packages.empty();
      });

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
      });

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
  rclcpp::Service<robot_catalog_msgs::srv::GetUrdf>::SharedPtr get_urdf_;
  rclcpp::Service<robot_catalog_msgs::srv::ResolveMesh>::SharedPtr resolve_mesh_;
};

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<CatalogServer>());
  rclcpp::shutdown();
  return 0;
}
