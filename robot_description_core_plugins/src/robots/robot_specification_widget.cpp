#include "robot_description_core_plugins/robots/robot_specification_widget.hpp"
#include <QApplication>
#include <QStyle>
#include <ament_index_cpp/get_package_prefix.hpp>

namespace robot_description::core_plugins
{
RobotSpecificationWidget::RobotSpecificationWidget(QWidget* parent)
  : QWidget(parent)
{
  setupLayout();
  clear();
}

void RobotSpecificationWidget::setupLayout()
{
  // Main layout
  main_layout_ = new QVBoxLayout(this);
  main_layout_->setContentsMargins(10, 10, 10, 10);
  main_layout_->setSpacing(15);
  
  // Create scroll area for long content
  scroll_area_ = new QScrollArea(this);
  scroll_area_->setWidgetResizable(true);
  scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scroll_area_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  
  content_widget_ = new QWidget();
  QVBoxLayout* content_layout = new QVBoxLayout(content_widget_);
  content_layout->setSpacing(15);
  
  // Basic robot information frame
  basic_info_frame_ = new QFrame();
  basic_info_frame_->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
  basic_info_frame_->setLineWidth(1);
  
  QVBoxLayout* basic_layout = new QVBoxLayout(basic_info_frame_);
  
  // Robot image
  robot_image_label_ = new QLabel();
  robot_image_label_->setAlignment(Qt::AlignCenter);
  robot_image_label_->setMinimumSize(150, 150);
  robot_image_label_->setMaximumSize(200, 200);
  robot_image_label_->setScaledContents(true);
  robot_image_label_->setStyleSheet("QLabel { border: 1px solid #ccc; background-color: #f9f9f9; }");
  basic_layout->addWidget(robot_image_label_);
  
  // Robot name
  robot_name_label_ = new QLabel();
  robot_name_label_->setAlignment(Qt::AlignCenter);
  robot_name_label_->setStyleSheet("QLabel { font-size: 18px; font-weight: bold; color: #333; }");
  robot_name_label_->setWordWrap(true);
  basic_layout->addWidget(robot_name_label_);
  
  // Robot description
  robot_description_label_ = new QLabel();
  robot_description_label_->setAlignment(Qt::AlignCenter);
  robot_description_label_->setWordWrap(true);
  robot_description_label_->setStyleSheet("QLabel { font-size: 12px; color: #666; }");
  basic_layout->addWidget(robot_description_label_);
  
  content_layout->addWidget(basic_info_frame_);
  
  // Specifications group
  specs_group_ = new QGroupBox("Technical Specifications");
  specs_group_->setStyleSheet("QGroupBox { font-weight: bold; }");
  specs_layout_ = new QGridLayout(specs_group_);
  specs_layout_->setColumnStretch(1, 1);
  content_layout->addWidget(specs_group_);
  
  // Package status group
  package_group_ = new QGroupBox("Package Dependencies");
  package_group_->setStyleSheet("QGroupBox { font-weight: bold; }");
  package_layout_ = new QVBoxLayout(package_group_);
  content_layout->addWidget(package_group_);
  
  // Add stretch to push content to top
  content_layout->addStretch();
  
  scroll_area_->setWidget(content_widget_);
  main_layout_->addWidget(scroll_area_);
}

void RobotSpecificationWidget::setRobotConfig(const RobotConfig& config)
{
  current_robot_ = config;
  
  // Update basic information
  robot_name_label_->setText(QString::fromStdString(config.display_name));
  robot_description_label_->setText(QString::fromStdString(config.description));
  
  // Load robot image
  if (std::filesystem::exists(config.image_path)) {
    QPixmap pixmap(QString::fromStdString(config.image_path.string()));
    if (!pixmap.isNull()) {
      robot_image_label_->setPixmap(pixmap);
    } else {
      robot_image_label_->setText("Image\nNot Available");
      robot_image_label_->setStyleSheet("QLabel { border: 1px dashed #999; color: #999; }");
    }
  } else {
    robot_image_label_->setText("Image\nNot Found");
    robot_image_label_->setStyleSheet("QLabel { border: 1px dashed #999; color: #999; }");
  }
  
  // Update specifications
  updateSpecifications(config.specifications);
  
  // Update package status
  updatePackageStatus(config.required_packages, config.optional_packages);
}

void RobotSpecificationWidget::updateSpecifications(const RobotSpecifications& specs)
{
  // Clear existing specifications
  QLayoutItem* item;
  while ((item = specs_layout_->takeAt(0)) != nullptr) {
    delete item->widget();
    delete item;
  }
  
  int row = 0;
  
  // Degrees of Freedom
  if (specs.degrees_of_freedom > 0) {
    createSpecificationRow("Degrees of Freedom:", 
                          QString::number(specs.degrees_of_freedom), 
                          specs_layout_, row++);
  }
  
  // Payload with visual bar
  if (specs.payload_kg > 0.0) {
    createSpecificationRow("Max Payload:", 
                          formatSpecValue(specs.payload_kg, "kg"), 
                          specs_layout_, row++);
    
    // Add payload bar (normalized to common industrial robot range 0-20kg)
    payload_bar_ = new QProgressBar();
    payload_bar_->setMinimum(0);
    payload_bar_->setMaximum(20);
    payload_bar_->setValue(static_cast<int>(std::min(specs.payload_kg, 20.0)));
    payload_bar_->setTextVisible(false);
    payload_bar_->setMaximumHeight(10);
    specs_layout_->addWidget(payload_bar_, row-1, 2);
  }
  
  // Reach with visual bar
  if (specs.reach_mm > 0.0) {
    createSpecificationRow("Max Reach:", 
                          formatSpecValue(specs.reach_mm, "mm"), 
                          specs_layout_, row++);
    
    // Add reach bar (normalized to common range 0-3000mm)
    reach_bar_ = new QProgressBar();
    reach_bar_->setMinimum(0);
    reach_bar_->setMaximum(3000);
    reach_bar_->setValue(static_cast<int>(std::min(specs.reach_mm, 3000.0)));
    reach_bar_->setTextVisible(false);
    reach_bar_->setMaximumHeight(10);
    specs_layout_->addWidget(reach_bar_, row-1, 2);
  }
  
  // Weight
  if (specs.weight_kg > 0.0) {
    createSpecificationRow("Weight:", 
                          formatSpecValue(specs.weight_kg, "kg"), 
                          specs_layout_, row++);
  }
  
  // Repeatability
  if (specs.repeatability_mm > 0.0) {
    createSpecificationRow("Repeatability:", 
                          formatSpecValue(specs.repeatability_mm, "mm"), 
                          specs_layout_, row++);
  }
  
  // Max Speed
  if (specs.max_speed_ms > 0.0) {
    createSpecificationRow("Max Speed:", 
                          formatSpecValue(specs.max_speed_ms, "m/s"), 
                          specs_layout_, row++);
  }
  
  // Mounting Options
  if (!specs.mounting_options.empty()) {
    QString mounting_text;
    for (size_t i = 0; i < specs.mounting_options.size(); ++i) {
      if (i > 0) mounting_text += ", ";
      mounting_text += QString::fromStdString(specs.mounting_options[i]);
    }
    createSpecificationRow("Mounting:", mounting_text, specs_layout_, row++);
  }
  
  // Boolean features
  QStringList features;
  if (specs.collaborative) features << "Collaborative";
  if (specs.safety_certified) features << "Safety Certified";
  if (specs.torque_sensing) features << "Torque Sensing";
  
  if (!features.isEmpty()) {
    createSpecificationRow("Features:", features.join(", "), specs_layout_, row++);
  }
}

void RobotSpecificationWidget::updatePackageStatus(const std::vector<std::string>& required, 
                                                  const std::vector<std::string>& optional)
{
  // Clear existing package status
  QLayoutItem* item;
  while ((item = package_layout_->takeAt(0)) != nullptr) {
    delete item->widget();
    delete item;
  }
  
  auto& config_manager = RobotConfigManager::getInstance();
  auto missing_packages = config_manager.getMissingPackages(current_robot_);
  
  // Required packages section
  if (!required.empty()) {
    QLabel* req_header = new QLabel("Required Packages:");
    req_header->setStyleSheet("QLabel { font-weight: bold; color: #333; }");
    package_layout_->addWidget(req_header);
    
    for (const auto& package : required) {
      QHBoxLayout* pkg_layout = new QHBoxLayout();
      
      // Package name
      QLabel* pkg_label = new QLabel(QString::fromStdString(package));
      pkg_layout->addWidget(pkg_label);
      
      // Status indicator
      bool is_missing = std::find(missing_packages.begin(), missing_packages.end(), package) 
                       != missing_packages.end();
      
      QLabel* status_label = new QLabel();
      if (is_missing) {
        status_label->setText("❌ Missing");
        status_label->setStyleSheet("QLabel { color: #d32f2f; font-weight: bold; }");
      } else {
        status_label->setText("✅ Installed");
        status_label->setStyleSheet("QLabel { color: #388e3c; font-weight: bold; }");
      }
      pkg_layout->addWidget(status_label);
      pkg_layout->addStretch();
      
      QWidget* pkg_widget = new QWidget();
      pkg_widget->setLayout(pkg_layout);
      package_layout_->addWidget(pkg_widget);
    }
  }
  
  // Optional packages section
  if (!optional.empty()) {
    QLabel* opt_header = new QLabel("Optional Packages:");
    opt_header->setStyleSheet("QLabel { font-weight: bold; color: #666; margin-top: 10px; }");
    package_layout_->addWidget(opt_header);
    
    for (const auto& package : optional) {
      QHBoxLayout* pkg_layout = new QHBoxLayout();
      
      QLabel* pkg_label = new QLabel(QString::fromStdString(package));
      pkg_label->setStyleSheet("QLabel { color: #666; }");
      pkg_layout->addWidget(pkg_label);
      
      // Check if optional package is installed
      try {
        ament_index_cpp::get_package_share_directory(package);
        QLabel* status_label = new QLabel("✅ Installed");
        status_label->setStyleSheet("QLabel { color: #388e3c; }");
        pkg_layout->addWidget(status_label);
      } catch (const ament_index_cpp::PackageNotFoundError&) {
        QLabel* status_label = new QLabel("⚪ Not Installed");
        status_label->setStyleSheet("QLabel { color: #999; }");
        pkg_layout->addWidget(status_label);
      }
      
      pkg_layout->addStretch();
      
      QWidget* pkg_widget = new QWidget();
      pkg_widget->setLayout(pkg_layout);
      package_layout_->addWidget(pkg_widget);
    }
  }
  
  package_layout_->addStretch();
}

void RobotSpecificationWidget::createSpecificationRow(const QString& label, const QString& value, 
                                                     QGridLayout* layout, int row)
{
  QLabel* label_widget = new QLabel(label);
  label_widget->setStyleSheet("QLabel { font-weight: bold; color: #333; }");
  
  QLabel* value_widget = new QLabel(value);
  value_widget->setStyleSheet("QLabel { color: #666; }");
  
  layout->addWidget(label_widget, row, 0);
  layout->addWidget(value_widget, row, 1);
}

QString RobotSpecificationWidget::formatSpecValue(double value, const QString& unit)
{
  // Format numbers nicely - remove unnecessary decimal places
  if (value == static_cast<int>(value)) {
    return QString::number(static_cast<int>(value)) + " " + unit;
  } else {
    return QString::number(value, 'f', 1) + " " + unit;
  }
}

void RobotSpecificationWidget::clear()
{
  robot_name_label_->setText("No Robot Selected");
  robot_description_label_->setText("Select a robot to view its specifications");
  robot_image_label_->clear();
  robot_image_label_->setText("No Image");
  robot_image_label_->setStyleSheet("QLabel { border: 1px dashed #ccc; color: #999; }");
  
  // Clear specifications
  QLayoutItem* item;
  while ((item = specs_layout_->takeAt(0)) != nullptr) {
    delete item->widget();
    delete item;
  }
  
  // Clear package status
  while ((item = package_layout_->takeAt(0)) != nullptr) {
    delete item->widget();
    delete item;
  }
}

} // namespace robot_description::core_plugins