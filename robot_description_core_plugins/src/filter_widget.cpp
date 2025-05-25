#include "robot_description_core_plugins/filter_widget.hpp"

namespace robot_description::core_plugins
{

FilterWidget::FilterWidget(QWidget* parent)
  : QWidget(parent), updating_filters_(false)
{
  setupLayout();
  connectSignals();
  clearFilters();
}

void FilterWidget::setupLayout()
{
  main_layout_ = new QVBoxLayout(this);
  main_layout_->setSpacing(15);
  main_layout_->setContentsMargins(10, 10, 10, 10);
  
  // Search group
  search_group_ = new QGroupBox("Search");
  QVBoxLayout* search_layout = new QVBoxLayout(search_group_);
  
  search_edit_ = new QLineEdit();
  search_edit_->setPlaceholderText("Search robots...");
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
  
  // Payload range
  specs_layout->addWidget(new QLabel("Payload (kg):"), 0, 0);
  
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
  
  specs_layout->addLayout(payload_layout, 0, 1);
  
  // Reach range
  specs_layout->addWidget(new QLabel("Reach (mm):"), 1, 0);
  
  QHBoxLayout* reach_layout = new QHBoxLayout();
  min_reach_spin_ = new QDoubleSpinBox();
  min_reach_spin_->setMinimum(0.0);
  min_reach_spin_->setMaximum(5000.0);
  min_reach_spin_->setDecimals(0);
  min_reach_spin_->setSuffix(" mm");
  min_reach_spin_->setSpecialValueText("Min");
  reach_layout->addWidget(min_reach_spin_);
  
  reach_layout->addWidget(new QLabel(" to "));
  
  max_reach_spin_ = new QDoubleSpinBox();
  max_reach_spin_->setMinimum(0.0);
  max_reach_spin_->setMaximum(5000.0);
  max_reach_spin_->setDecimals(0);
  max_reach_spin_->setSuffix(" mm");
  max_reach_spin_->setSpecialValueText("Max");
  reach_layout->addWidget(max_reach_spin_);
  
  specs_layout->addLayout(reach_layout, 1, 1);
  
  // Degrees of freedom
  specs_layout->addWidget(new QLabel("DOF:"), 2, 0);
  dof_combo_ = new QComboBox();
  dof_combo_->addItem("Any", 0);
  dof_combo_->addItem("6 DOF", 6);
  dof_combo_->addItem("7 DOF", 7);
  specs_layout->addWidget(dof_combo_, 2, 1);
  
  main_layout_->addWidget(specs_group_);
  
  // Features group
  features_group_ = new QGroupBox("Features");
  QVBoxLayout* features_layout = new QVBoxLayout(features_group_);
  
  collaborative_check_ = new QCheckBox("Collaborative robots only");
  features_layout->addWidget(collaborative_check_);
  
  main_layout_->addWidget(features_group_);
  
  // Clear button
  clear_button_ = new QPushButton("Clear All Filters");
  main_layout_->addWidget(clear_button_);
  
  // Add stretch to push everything to top
  main_layout_->addStretch();
}

void FilterWidget::connectSignals()
{
  connect(search_edit_, &QLineEdit::textChanged, this, &FilterWidget::onFilterControlChanged);
  connect(category_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), 
          this, &FilterWidget::onFilterControlChanged);
  connect(min_payload_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
          this, &FilterWidget::onFilterControlChanged);
  connect(max_payload_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
          this, &FilterWidget::onFilterControlChanged);
  connect(min_reach_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
          this, &FilterWidget::onFilterControlChanged);
  connect(max_reach_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
          this, &FilterWidget::onFilterControlChanged);
  connect(dof_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), 
          this, &FilterWidget::onFilterControlChanged);
  connect(collaborative_check_, &QCheckBox::toggled, this, &FilterWidget::onFilterControlChanged);
  
  connect(clear_button_, &QPushButton::clicked, this, &FilterWidget::onClearFiltersClicked);
}

RobotFilter FilterWidget::getCurrentFilter() const
{
  RobotFilter filter;
  
  // Search text
  filter.search_text = search_edit_->text().toStdString();
  
  // Category
  QString category_data = category_combo_->currentData().toString();
  if (!category_data.isEmpty()) {
    filter.category = category_data.toStdString();
  }
  
  // Payload range
  if (min_payload_spin_->value() > 0.0) {
    filter.min_payload = min_payload_spin_->value();
  }
  if (max_payload_spin_->value() > 0.0) {
    filter.max_payload = max_payload_spin_->value();
  }
  
  // Reach range
  if (min_reach_spin_->value() > 0.0) {
    filter.min_reach = min_reach_spin_->value();
  }
  if (max_reach_spin_->value() > 0.0) {
    filter.max_reach = max_reach_spin_->value();
  }
  
  // Degrees of freedom
  int dof_value = dof_combo_->currentData().toInt();
  if (dof_value > 0) {
    filter.degrees_of_freedom = dof_value;
  }
  
  // Collaborative
  if (collaborative_check_->isChecked()) {
    filter.collaborative_only = true;
  }
  
  return filter;
}

void FilterWidget::setCategories(const std::vector<CategoryInfo>& categories)
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

void FilterWidget::clearFilters()
{
  updating_filters_ = true;
  
  search_edit_->clear();
  category_combo_->setCurrentIndex(0);
  min_payload_spin_->setValue(0.0);
  max_payload_spin_->setValue(0.0);
  min_reach_spin_->setValue(0.0);
  max_reach_spin_->setValue(0.0);
  dof_combo_->setCurrentIndex(0);
  collaborative_check_->setChecked(false);
  
  updating_filters_ = false;
  
  onFilterControlChanged();
}

void FilterWidget::onFilterControlChanged()
{
  if (updating_filters_) return;
  
  RobotFilter filter = getCurrentFilter();
  Q_EMIT filterChanged(filter);
}

void FilterWidget::onClearFiltersClicked()
{
  clearFilters();
}

} // namespace robot_description::core_plugins