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
#include "robot_config_manager.hpp"

namespace robot_description::core_plugins
{

class FilterWidget : public QWidget
{
  Q_OBJECT
public:
  explicit FilterWidget(QWidget* parent = nullptr);
  RobotFilter getCurrentFilter() const;
  void setCategories(const std::vector<CategoryInfo>& categories);
  void clearFilters();

Q_SIGNALS:
  void filterChanged(const RobotFilter& filter);

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
  
  // Filter controls
  QLineEdit* search_edit_;
  QComboBox* category_combo_;
  QDoubleSpinBox* min_payload_spin_;
  QDoubleSpinBox* max_payload_spin_;
  QDoubleSpinBox* min_reach_spin_;
  QDoubleSpinBox* max_reach_spin_;
  QComboBox* dof_combo_;
  QCheckBox* collaborative_check_;
  QPushButton* clear_button_;
  
  // Internal state
  bool updating_filters_;
};

} // namespace robot_description::core_plugins