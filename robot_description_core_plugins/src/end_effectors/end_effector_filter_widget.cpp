#include "robot_description_core_plugins/end_effectors/end_effector_filter_widget.hpp"

namespace robot_description::core_plugins
{

EndEffectorFilterWidget::EndEffectorFilterWidget(QWidget* parent)
  : QWidget(parent), updating_filters_(false)
{
  setupLayout();
  connectSignals();
  clearFilters();
}

void EndEffectorFilterWidget::setupLayout()
{
  main_layout_ = new QVBoxLayout(this);
  main_layout_->setSpacing(15);
  main_layout_->setContentsMargins(10, 10, 10, 10);
  
  // Search group
  search_group_ = new QGroupBox("Search");
  QVBoxLayout* search_layout = new QVBoxLayout(search_group_);
  
  search_edit_ = new QLineEdit();
  search_edit_->setPlaceholderText("Search end-effectors...");
  search_edit_->setClearButtonEnabled(true);
  search_layout->addWidget(search_edit_);
  
  main_layout_->addWidget(search_group_);
  
  // Category group
  category_group_ = new QGroupBox("Category");
  QVBoxLayout* category_layout = new QVBoxLayout(category_group_);
  
  category_combo_ = new QComboBox();
  category_combo_->addItem("All Categories", "");
  category_layout->addWidget(category_combo_);
  
  main_layout_->addWidget(category_group_);
  
  // Specifications group
  specs_group_ = new QGroupBox("Specifications");
  QGridLayout* specs_layout = new QGridLayout(specs_group_);
  
  // Opening range
  specs_layout->addWidget(new QLabel("Opening (mm):"), 0, 0);
  
  QHBoxLayout* opening_layout = new QHBoxLayout();
  min_opening_spin_ = new QDoubleSpinBox();
  min_opening_spin_->setMinimum(0.0);
  min_opening_spin_->setMaximum(500.0);
  min_opening_spin_->setDecimals(1);
  min_opening_spin_->setSuffix(" mm");
  min_opening_spin_->setSpecialValueText("Min");
  opening_layout->addWidget(min_opening_spin_);
  
  opening_layout->addWidget(new QLabel(" to "));
  
  max_opening_spin_ = new QDoubleSpinBox();
  max_opening_spin_->setMinimum(0.0);
  max_opening_spin_->setMaximum(500.0);
  max_opening_spin_->setDecimals(1);
  max_opening_spin_->setSuffix(" mm");
  max_opening_spin_->setSpecialValueText("Max");
  opening_layout->addWidget(max_opening_spin_);
  
  specs_layout->addLayout(opening_layout, 0, 1);
  
  // Force range
  specs_layout->addWidget(new QLabel("Force (N):"), 1, 0);
  
  QHBoxLayout* force_layout = new QHBoxLayout();
  min_force_spin_ = new QDoubleSpinBox();
  min_force_spin_->setMinimum(0.0);
  min_force_spin_->setMaximum(2000.0);
  min_force_spin_->setDecimals(0);
  min_force_spin_->setSuffix(" N");
  min_force_spin_->setSpecialValueText("Min");
  force_layout->addWidget(min_force_spin_);
  
  force_layout->addWidget(new QLabel(" to "));
  
  max_force_spin_ = new QDoubleSpinBox();
  max_force_spin_->setMinimum(0.0);
  max_force_spin_->setMaximum(2000.0);
  max_force_spin_->setDecimals(0);
  max_force_spin_->setSuffix(" N");
  max_force_spin_->setSpecialValueText("Max");
  force_layout->addWidget(max_force_spin_);
  
  specs_layout->addLayout(force_layout, 1, 1);
  
  // Payload range
  specs_layout->addWidget(new QLabel("Payload (kg):"), 2, 0);
  
  QHBoxLayout* payload_layout = new QHBoxLayout();
  min_payload_spin_ = new QDoubleSpinBox();
  min_payload_spin_->setMinimum(0.0);
  min_payload_spin_->setMaximum(100.0);
  min_payload_spin_->setDecimals(1);
  min_payload_spin_->setSuffix(" kg");
  min_payload_spin_->setSpecialValueText("Min");
  payload_layout->addWidget(min_payload_spin_);
  
  payload_layout->addWidget(new QLabel(" to "));
  
  max_payload_spin_ = new QDoubleSpinBox();
  max_payload_spin_->setMinimum(0.0);
  max_payload_spin_->setMaximum(100.0);
  max_payload_spin_->setDecimals(1);
  max_payload_spin_->setSuffix(" kg");
  max_payload_spin_->setSpecialValueText("Max");
  payload_layout->addWidget(max_payload_spin_);
  
  specs_layout->addLayout(payload_layout, 2, 1);
  
  // Actuation type
  specs_layout->addWidget(new QLabel("Actuation:"), 3, 0);
  actuation_type_combo_ = new QComboBox();
  actuation_type_combo_->addItem("Any", "");
  actuation_type_combo_->addItem("Electric", "electric");
  actuation_type_combo_->addItem("Pneumatic", "pneumatic");
  actuation_type_combo_->addItem("Hydraulic", "hydraulic");
  specs_layout->addWidget(actuation_type_combo_, 3, 1);
  
  // Interface type
  specs_layout->addWidget(new QLabel("Interface:"), 4, 0);
  interface_type_combo_ = new QComboBox();
  interface_type_combo_->addItem("Any", "");
  interface_type_combo_->addItem("Modbus", "modbus");
  interface_type_combo_->addItem("I/O", "io");
  interface_type_combo_->addItem("Ethernet", "ethernet");
  interface_type_combo_->addItem("CAN Bus", "canbus");
  interface_type_combo_->addItem("Custom", "custom");
  specs_layout->addWidget(interface_type_combo_, 4, 1);
  
  main_layout_->addWidget(specs_group_);
  
  // Features group
  features_group_ = new QGroupBox("Required Features");
  QVBoxLayout* features_layout = new QVBoxLayout(features_group_);
  
  force_feedback_check_ = new QCheckBox("Force Feedback");
  features_layout->addWidget(force_feedback_check_);
  
  position_feedback_check_ = new QCheckBox("Position Feedback");
  features_layout->addWidget(position_feedback_check_);
  
  adaptive_grip_check_ = new QCheckBox("Adaptive Grip");
  features_layout->addWidget(adaptive_grip_check_);
  
  main_layout_->addWidget(features_group_);
  
  // Compatibility group
  compatibility_group_ = new QGroupBox("Robot Compatibility");
  QVBoxLayout* compatibility_layout = new QVBoxLayout(compatibility_group_);
  
  robot_brand_combo_ = new QComboBox();
  robot_brand_combo_->addItem("Any Robot", "");
  robot_brand_combo_->addItem("Universal Robots", "universal_robots");
  robot_brand_combo_->addItem("KUKA", "kuka");
  robot_brand_combo_->addItem("ABB", "abb");
  robot_brand_combo_->addItem("FANUC", "fanuc");
  robot_brand_combo_->addItem("Collaborative Robots", "collaborative_robots");
  compatibility_layout->addWidget(robot_brand_combo_);
  
  main_layout_->addWidget(compatibility_group_);
  
  // Clear button
  clear_button_ = new QPushButton("Clear All Filters");
  main_layout_->addWidget(clear_button_);
  
  // Add stretch to push everything to top
  main_layout_->addStretch();
}

void EndEffectorFilterWidget::connectSignals()
{
  connect(search_edit_, &QLineEdit::textChanged, this, &EndEffectorFilterWidget::onFilterControlChanged);
  connect(category_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), 
          this, &EndEffectorFilterWidget::onFilterControlChanged);
  
  connect(min_opening_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
          this, &EndEffectorFilterWidget::onFilterControlChanged);
  connect(max_opening_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
          this, &EndEffectorFilterWidget::onFilterControlChanged);
  connect(min_force_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
          this, &EndEffectorFilterWidget::onFilterControlChanged);
  connect(max_force_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
          this, &EndEffectorFilterWidget::onFilterControlChanged);
  connect(min_payload_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
          this, &EndEffectorFilterWidget::onFilterControlChanged);
  connect(max_payload_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
          this, &EndEffectorFilterWidget::onFilterControlChanged);
  
  connect(actuation_type_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), 
          this, &EndEffectorFilterWidget::onFilterControlChanged);
  connect(interface_type_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), 
          this, &EndEffectorFilterWidget::onFilterControlChanged);
  
  connect(force_feedback_check_, &QCheckBox::toggled, this, &EndEffectorFilterWidget::onFilterControlChanged);
  connect(position_feedback_check_, &QCheckBox::toggled, this, &EndEffectorFilterWidget::onFilterControlChanged);
  connect(adaptive_grip_check_, &QCheckBox::toggled, this, &EndEffectorFilterWidget::onFilterControlChanged);
  
  connect(robot_brand_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), 
          this, &EndEffectorFilterWidget::onFilterControlChanged);
  
  connect(clear_button_, &QPushButton::clicked, this, &EndEffectorFilterWidget::onClearFiltersClicked);
}

EndEffectorFilter EndEffectorFilterWidget::getCurrentFilter() const
{
  EndEffectorFilter filter;
  
  // Search text
  filter.search_text = search_edit_->text().toStdString();
  
  // Category
  QString category_data = category_combo_->currentData().toString();
  if (!category_data.isEmpty()) {
    filter.category = category_data.toStdString();
  }
  
  // Opening range
  if (min_opening_spin_->value() > 0.0) {
    filter.min_opening = min_opening_spin_->value();
  }
  if (max_opening_spin_->value() > 0.0) {
    filter.max_opening = max_opening_spin_->value();
  }
  
  // Force range
  if (min_force_spin_->value() > 0.0) {
    filter.min_force = min_force_spin_->value();
  }
  if (max_force_spin_->value() > 0.0) {
    filter.max_force = max_force_spin_->value();
  }
  
  // Payload range
  if (min_payload_spin_->value() > 0.0) {
    filter.min_payload = min_payload_spin_->value();
  }
  if (max_payload_spin_->value() > 0.0) {
    filter.max_payload = max_payload_spin_->value();
  }
  
  // Actuation type
  QString actuation_data = actuation_type_combo_->currentData().toString();
  if (!actuation_data.isEmpty()) {
    filter.actuation_type = actuation_data.toStdString();
  }
  
  // Interface type
  QString interface_data = interface_type_combo_->currentData().toString();
  if (!interface_data.isEmpty()) {
    filter.interface_type = interface_data.toStdString();
  }
  
  // Feature requirements
  if (force_feedback_check_->isChecked()) {
    filter.force_feedback_required = true;
  }
  if (position_feedback_check_->isChecked()) {
    filter.position_feedback_required = true;
  }
  if (adaptive_grip_check_->isChecked()) {
    filter.adaptive_grip_required = true;
  }
  
  // Robot brand compatibility
  QString robot_brand_data = robot_brand_combo_->currentData().toString();
  if (!robot_brand_data.isEmpty()) {
    filter.compatible_robot_brand = robot_brand_data.toStdString();
  }
  
  return filter;
}

void EndEffectorFilterWidget::setCategories(const std::vector<EndEffectorCategoryInfo>& categories)
{
  updating_filters_ = true;
  
  // Clear existing categories (keep "All Categories")
  while (category_combo_->count() > 1) {
    category_combo_->removeItem(1);
  }
  
  // Add new categories
  for (const auto& category : categories) {
    category_combo_->addItem(QString::fromStdString(category.display_name), 
                            QString::fromStdString(category.id));
  }
  
  updating_filters_ = false;
}

void EndEffectorFilterWidget::setCompatibleRobotBrand(const std::string& robot_brand)
{
  updating_filters_ = true;
  
  // Find and select the robot brand in the combo box
  for (int i = 0; i < robot_brand_combo_->count(); ++i) {
    if (robot_brand_combo_->itemData(i).toString().toStdString() == robot_brand) {
      robot_brand_combo_->setCurrentIndex(i);
      break;
    }
  }
  
  updating_filters_ = false;
  onFilterControlChanged();
}

void EndEffectorFilterWidget::clearFilters()
{
  updating_filters_ = true;
  
  search_edit_->clear();
  category_combo_->setCurrentIndex(0);
  min_opening_spin_->setValue(0.0);
  max_opening_spin_->setValue(0.0);
  min_force_spin_->setValue(0.0);
  max_force_spin_->setValue(0.0);
  min_payload_spin_->setValue(0.0);
  max_payload_spin_->setValue(0.0);
  actuation_type_combo_->setCurrentIndex(0);
  interface_type_combo_->setCurrentIndex(0);
  force_feedback_check_->setChecked(false);
  position_feedback_check_->setChecked(false);
  adaptive_grip_check_->setChecked(false);
  robot_brand_combo_->setCurrentIndex(0);
  
  updating_filters_ = false;
  
  onFilterControlChanged();
}

void EndEffectorFilterWidget::onFilterControlChanged()
{
  if (updating_filters_) return;
  
  EndEffectorFilter filter = getCurrentFilter();
  Q_EMIT filterChanged(filter);
}

void EndEffectorFilterWidget::onClearFiltersClicked()
{
  clearFilters();
}

} // namespace robot_description::core_plugins