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
#include "end_effector_config_manager.hpp"

namespace robot_description::core_plugins
{

class EndEffectorSpecificationWidget : public QWidget
{
  Q_OBJECT
public:
  explicit EndEffectorSpecificationWidget(QWidget* parent = nullptr);
  void setEndEffectorConfig(const EndEffectorConfig& config);
  void clear();

private:
  void setupLayout();
  void updateSpecifications(const EndEffectorSpecifications& specs);
  void updatePackageStatus(const std::vector<std::string>& required, 
                          const std::vector<std::string>& optional);
  void updateCompatibilityInfo(const EndEffectorConfig& config);
  void createSpecificationRow(const QString& label, const QString& value, 
                             QGridLayout* layout, int row);
  QString formatSpecValue(double value, const QString& unit);
  QString formatActuationType(const std::string& actuation_type);
  QString formatInterfaceType(const std::string& interface_type);
  
  // Main layout components
  QVBoxLayout* main_layout_;
  QScrollArea* scroll_area_;
  QWidget* content_widget_;
  
  // End-effector basic info
  QLabel* end_effector_name_label_;
  QLabel* end_effector_description_label_;
  QLabel* end_effector_image_label_;
  QLabel* manufacturer_label_;
  QFrame* basic_info_frame_;
  
  // Specification display widgets
  QGroupBox* specs_group_;
  QGridLayout* specs_layout_;
  
  // Capability indicators
  QGroupBox* capabilities_group_;
  QVBoxLayout* capabilities_layout_;
  
  // Visual progress bars for key specs
  QProgressBar* opening_bar_;
  QProgressBar* force_bar_;
  QProgressBar* payload_bar_;
  
  // Compatibility information
  QGroupBox* compatibility_group_;
  QVBoxLayout* compatibility_layout_;
  
  // Package status indicators
  QGroupBox* package_group_;
  QVBoxLayout* package_layout_;
  
  // Current end-effector config
  EndEffectorConfig current_end_effector_;
};

} // namespace robot_description::core_plugins