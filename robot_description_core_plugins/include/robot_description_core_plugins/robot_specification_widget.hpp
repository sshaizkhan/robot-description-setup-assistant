#pragma once

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QScrollArea>
#include <QGroupBox>
#include <QPushButton>
#include <QPixmap>
#include "robot_description_core_plugins/robot_config_manager.hpp"

namespace robot_description::core_plugins
{

class RobotSpecificationWidget : public QWidget
{
  Q_OBJECT
public:
  explicit RobotSpecificationWidget(QWidget* parent = nullptr);
  void setRobotConfig(const RobotConfig& config);
  void clear();

private:
  void setupLayout();
  void updateSpecifications(const RobotSpecifications& specs);
  void updatePackageStatus(const std::vector<std::string>& required, 
                          const std::vector<std::string>& optional);
  void createSpecificationRow(const QString& label, const QString& value, 
                             QGridLayout* layout, int row);
  QString formatSpecValue(double value, const QString& unit);
  
  // Main layout components
  QVBoxLayout* main_layout_;
  QScrollArea* scroll_area_;
  QWidget* content_widget_;
  
  // Robot basic info
  QLabel* robot_name_label_;
  QLabel* robot_description_label_;
  QLabel* robot_image_label_;
  QFrame* basic_info_frame_;
  
  // Specification display widgets
  QGroupBox* specs_group_;
  QGridLayout* specs_layout_;
  QLabel* dof_label_;
  QLabel* payload_label_;
  QLabel* reach_label_;
  QLabel* weight_label_;
  QLabel* repeatability_label_;
  QLabel* max_speed_label_;
  QLabel* mounting_label_;
  QLabel* collaborative_label_;
  
  // Visual progress bars for key specs
  QProgressBar* payload_bar_;
  QProgressBar* reach_bar_;
  
  // Package status indicators
  QGroupBox* package_group_;
  QVBoxLayout* package_layout_;
  
  // Current robot config
  RobotConfig current_robot_;
};

} // namespace robot_description::core_plugins