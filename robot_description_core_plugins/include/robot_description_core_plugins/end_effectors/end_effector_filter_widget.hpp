#pragma once

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include "end_effector_config_manager.hpp"

namespace robot_description::core_plugins
{

class EndEffectorFilterWidget : public QWidget
{
  Q_OBJECT
public:
  explicit EndEffectorFilterWidget(QWidget* parent = nullptr);
  EndEffectorFilter getCurrentFilter() const;
  void setCategories(const std::vector<EndEffectorCategoryInfo>& categories);
  void clearFilters();
  void setCompatibleRobotBrand(const std::string& robot_brand);

Q_SIGNALS:
  void filterChanged(const EndEffectorFilter& filter);

private Q_SLOTS:
  void onFilterControlChanged();
  void onClearFiltersClicked();

private:
  void setupLayout();
  void connectSignals();
  
  // Layout components
  QVBoxLayout* main_layout_;
  QGroupBox* search_group_;
  QGroupBox* category_group_;
  QGroupBox* specs_group_;
  QGroupBox* features_group_;
  QGroupBox* compatibility_group_;
  
  // Filter controls
  QLineEdit* search_edit_;
  QComboBox* category_combo_;
  
  // Specification filters
  QDoubleSpinBox* min_opening_spin_;
  QDoubleSpinBox* max_opening_spin_;
  QDoubleSpinBox* min_force_spin_;
  QDoubleSpinBox* max_force_spin_;
  QDoubleSpinBox* min_payload_spin_;
  QDoubleSpinBox* max_payload_spin_;
  
  // Type filters
  QComboBox* actuation_type_combo_;
  QComboBox* interface_type_combo_;
  
  // Feature filters
  QCheckBox* force_feedback_check_;
  QCheckBox* position_feedback_check_;
  QCheckBox* adaptive_grip_check_;
  
  // Compatibility filters
  QComboBox* robot_brand_combo_;
  
  QPushButton* clear_button_;
  
  // Internal state
  bool updating_filters_;
};

} // namespace robot_description::core_plugins