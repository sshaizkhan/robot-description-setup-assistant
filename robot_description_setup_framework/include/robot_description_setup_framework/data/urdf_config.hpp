#pragma once

#include <filesystem>
#include <memory>
#include <robot_description_setup_framework/config.hpp>
#include <string>
#include <vector>
#include <yaml-cpp/node/node.h>
#if __has_include(<urdf/model.hpp>) // for testing a valid urdf is loaded
#include <urdf/model.hpp>
#else
#include <urdf/model.h>
#endif

namespace robot_description::setup_framework {

class URDFConfig : public SetupConfig {
public:
  URDFConfig() { urdf_model_ = std::make_shared<urdf::Model>(); }

  void onInit() override;

  void loadPrevious(const std::filesystem::path &package_path,
                    const YAML::Node &node) override;
  YAML::Node saveToYaml() const override;

  // Load URDF file
  void loadFromPath(const std::filesystem::path &urdf_file_path,
                    const std::string &xacro_args = "");
  void loadFromPath(const std::filesystem::path &urdf_file_path,
                    const std::vector<std::string> &xacro_args);
  void loadFromPackage(const std::filesystem::path &package_name,
                       const std::filesystem::path &relative_path,
                       const std::string &xacro_args = "");

  const urdf::Model &getMode() const { return *urdf_model_; }

  const std::shared_ptr<urdf::Model> &getModelPtr() const {
    return urdf_model_;
  }

  std::string getURDFPackageName() const { return urdf_package_name_; }

  std::string getURDFContents() const { return urdf_string_; }

  std::filesystem::path getURDFPath() const { return urdf_path_; }

  std::string getXacroArgs() const { return xacro_args_; }

  bool isConfgigured() const override;

  bool isXacroFile() const;

  std::string getRobotName() const { return urdf_model_->getName(); }

protected:
  void setPackageName();
  void load();

  // full file-system path to urdf
  std::filesystem::path urdf_path_;

  // name of the package containing urdf (can be empty)
  std::string urdf_package_name_;

  // xacro arguments in two different formats
  std::string xacro_args_;
  std::vector<std::string> xacro_args_vec_;

  /// URDF robot model
  std::shared_ptr<urdf::Model> urdf_model_;

  // URDF robot model string
  std::string urdf_string_;
};
} // namespace robot_description::setup_framework