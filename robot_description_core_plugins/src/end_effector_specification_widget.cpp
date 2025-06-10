/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2025, Shahwaz Khan
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Shahwaz Khan nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/
/* Author: Shahwaz Khan */

#include "robot_description_core_plugins/end_effector_specification_widget.hpp"
#include <QApplication>
#include <QStyle>
#include <ament_index_cpp/get_package_prefix.hpp>

namespace robot_description::core_plugins
{
EndEffectorSpecificationWidget::EndEffectorSpecificationWidget(QWidget* parent)
  : QWidget(parent)
{
  setupLayout();
  clear();
}

void EndEffectorSpecificationWidget::setupLayout()
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
  
  // Basic end-effector information frame
  basic_info_frame_ = new QFrame();
  basic_info_frame_->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
  basic_info_frame_->setLineWidth(1);
  
  QVBoxLayout* basic_layout = new QVBoxLayout(basic_info_frame_);
  
  // End-effector image
  end_effector_image_label_ = new QLabel();
  end_effector_image_label_->setAlignment(Qt::AlignCenter);
  end_effector_image_label_->setMinimumSize(150, 150);
  end_effector_image_label_->setMaximumSize(200, 200);
  end_effector_image_label_->setScaledContents(true);
  end_effector_image_label_->setStyleSheet("QLabel { border: 1px solid #ccc; background-color: #f9f9f9; }");
  basic_layout->addWidget(end_effector_image_label_);
  
  // End-effector name
  end_effector_name_label_ = new QLabel();
  end_effector_name_label_->setAlignment(Qt::AlignCenter);
  end_effector_name_label_->setStyleSheet("QLabel { font-size: 18px; font-weight: bold; color: #333; }");
  end_effector_name_label_->setWordWrap(true);
  basic_layout->addWidget(end_effector_name_label_);
  
  // Manufacturer
  manufacturer_label_ = new QLabel();
  manufacturer_label_->setAlignment(Qt::AlignCenter);
  manufacturer_label_->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; color: #666; }");
  basic_layout->addWidget(manufacturer_label_);
  
  // End-effector description
  end_effector_description_label_ = new QLabel();
  end_effector_description_label_->setAlignment(Qt::AlignCenter);
  end_effector_description_label_->setWordWrap(true);
  end_effector_description_label_->setStyleSheet("QLabel { font-size: 12px; color: #666; }");
  basic_layout->addWidget(end_effector_description_label_);
  
  content_layout->addWidget(basic_info_frame_);
  
  // Specifications group
  specs_group_ = new QGroupBox("Technical Specifications");
  specs_group_->setStyleSheet("QGroupBox { font-weight: bold; }");
  specs_layout_ = new QGridLayout(specs_group_);
  specs_layout_->setColumnStretch(1, 1);
  content_layout->addWidget(specs_group_);
  
  // Capabilities group
  capabilities_group_ = new QGroupBox("Capabilities");
  capabilities_group_->setStyleSheet("QGroupBox { font-weight: bold; }");
  capabilities_layout_ = new QVBoxLayout(capabilities_group_);
  content_layout->addWidget(capabilities_group_);
  
  // Compatibility group
  compatibility_group_ = new QGroupBox("Robot Compatibility");
  compatibility_group_->setStyleSheet("QGroupBox { font-weight: bold; }");
  compatibility_layout_ = new QVBoxLayout(compatibility_group_);
  content_layout->addWidget(compatibility_group_);
  
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

void EndEffectorSpecificationWidget::setEndEffectorConfig(const EndEffectorConfig& config)
{
  current_end_effector_ = config;
  
  // Update basic information
  end_effector_name_label_->setText(QString::fromStdString(config.display_name));
  manufacturer_label_->setText(QString::fromStdString(config.manufacturer));
  end_effector_description_label_->setText(QString::fromStdString(config.description));
  
  // Load end-effector image
  if (std::filesystem::exists(config.image_path)) {
    QPixmap pixmap(QString::fromStdString(config.image_path.string()));
    if (!pixmap.isNull()) {
      end_effector_image_label_->setPixmap(pixmap);
    } else {
      end_effector_image_label_->setText("Image\nNot Available");
      end_effector_image_label_->setStyleSheet("QLabel { border: 1px dashed #999; color: #999; }");
    }
  } else {
    end_effector_image_label_->setText("Image\nNot Found");
    end_effector_image_label_->setStyleSheet("QLabel { border: 1px dashed #999; color: #999; }");
  }
  
  // Update specifications
  updateSpecifications(config.specifications);
  
  // Update compatibility info
  updateCompatibilityInfo(config);
  
  // Update package status
  updatePackageStatus(config.required_packages, config.optional_packages);
}

void EndEffectorSpecificationWidget::updateSpecifications(const EndEffectorSpecifications& specs)
{
  // Clear existing specifications
  QLayoutItem* item;
  while ((item = specs_layout_->takeAt(0)) != nullptr) {
    delete item->widget();
    delete item;
  }
  
  int row = 0;
  
  // Maximum Opening with visual bar
  if (specs.max_opening_mm > 0.0) {
    createSpecificationRow("Max Opening:", 
                          formatSpecValue(specs.max_opening_mm, "mm"), 
                          specs_layout_, row++);
    
    // Add opening bar (normalized to 0-200mm range)
    opening_bar_ = new QProgressBar();
    opening_bar_->setMinimum(0);
    opening_bar_->setMaximum(200);
    opening_bar_->setValue(static_cast<int>(std::min(specs.max_opening_mm, 200.0)));
    opening_bar_->setTextVisible(false);
    opening_bar_->setMaximumHeight(10);
    opening_bar_->setStyleSheet("QProgressBar::chunk { background-color: #4CAF50; }");
    specs_layout_->addWidget(opening_bar_, row-1, 2);
  }
  
  // Maximum Force with visual bar
  if (specs.max_force_n > 0.0) {
    createSpecificationRow("Max Force:", 
                          formatSpecValue(specs.max_force_n, "N"), 
                          specs_layout_, row++);
    
    // Add force bar (normalized to 0-1000N range)
    force_bar_ = new QProgressBar();
    force_bar_->setMinimum(0);
    force_bar_->setMaximum(1000);
    force_bar_->setValue(static_cast<int>(std::min(specs.max_force_n, 1000.0)));
    force_bar_->setTextVisible(false);
    force_bar_->setMaximumHeight(10);
    force_bar_->setStyleSheet("QProgressBar::chunk { background-color: #FF5722; }");
    specs_layout_->addWidget(force_bar_, row-1, 2);
  }
  
  // Maximum Speed
  if (specs.max_speed_mms > 0.0) {
    createSpecificationRow("Max Speed:", 
                          formatSpecValue(specs.max_speed_mms, "mm/s"), 
                          specs_layout_, row++);
  }
  
  // Weight
  if (specs.weight_kg > 0.0) {
    createSpecificationRow("Weight:", 
                          formatSpecValue(specs.weight_kg, "kg"), 
                          specs_layout_, row++);
  }
  
  // Payload with visual bar
  if (specs.payload_kg > 0.0) {
    createSpecificationRow("Max Payload:", 
                          formatSpecValue(specs.payload_kg, "kg"), 
                          specs_layout_, row++);
    
    // Add payload bar (normalized to 0-50kg range)
    payload_bar_ = new QProgressBar();
    payload_bar_->setMinimum(0);
    payload_bar_->setMaximum(50);
    payload_bar_->setValue(static_cast<int>(std::min(specs.payload_kg, 50.0)));
    payload_bar_->setTextVisible(false);
    payload_bar_->setMaximumHeight(10);
    payload_bar_->setStyleSheet("QProgressBar::chunk { background-color: #2196F3; }");
    specs_layout_->addWidget(payload_bar_, row-1, 2);
  }
  
  // Actuation Type
  if (!specs.actuation_type.empty()) {
    createSpecificationRow("Actuation:", 
                          formatActuationType(specs.actuation_type), 
                          specs_layout_, row++);
  }
  
  // Interface Type
  if (!specs.interface_type.empty()) {
    createSpecificationRow("Interface:", 
                          formatInterfaceType(specs.interface_type), 
                          specs_layout_, row++);
  }
  
  // Update capabilities section
  QLayoutItem* cap_item;
  while ((cap_item = capabilities_layout_->takeAt(0)) != nullptr) {
    delete cap_item->widget();
    delete cap_item;
  }
  
  // Feature indicators
  QStringList capabilities;
  if (specs.force_feedback) capabilities << "✅ Force Feedback";
  else capabilities << "❌ Force Feedback";
  
  if (specs.position_feedback) capabilities << "✅ Position Feedback";
  else capabilities << "❌ Position Feedback";
  
  if (specs.adaptive_grip) capabilities << "✅ Adaptive Grip";
  else capabilities << "❌ Adaptive Grip";
  
  for (const QString& capability : capabilities) {
    QLabel* cap_label = new QLabel(capability);
    cap_label->setStyleSheet("QLabel { font-size: 12px; padding: 2px; }");
    capabilities_layout_->addWidget(cap_label);
  }
  
  // Compatible objects
  if (!specs.compatible_objects.empty()) {
    QLabel* objects_header = new QLabel("Compatible Objects:");
    objects_header->setStyleSheet("QLabel { font-weight: bold; color: #333; margin-top: 10px; }");
    capabilities_layout_->addWidget(objects_header);
    
    QString objects_text;
    for (size_t i = 0; i < specs.compatible_objects.size(); ++i) {
      if (i > 0) objects_text += ", ";
      objects_text += QString::fromStdString(specs.compatible_objects[i]);
    }
    
    QLabel* objects_label = new QLabel(objects_text);
    objects_label->setWordWrap(true);
    objects_label->setStyleSheet("QLabel { color: #666; }");
    capabilities_layout_->addWidget(objects_label);
  }
  
  capabilities_layout_->addStretch();
}

void EndEffectorSpecificationWidget::updateCompatibilityInfo(const EndEffectorConfig& config)
{
  // Clear existing compatibility info
  QLayoutItem* item;
  while ((item = compatibility_layout_->takeAt(0)) != nullptr) {
    delete item->widget();
    delete item;
  }
  
  // Mounting interface
  if (!config.mounting_interface.empty()) {
    QLabel* mount_header = new QLabel("Mounting Interface:");
    mount_header->setStyleSheet("QLabel { font-weight: bold; color: #333; }");
    compatibility_layout_->addWidget(mount_header);
    
    QLabel* mount_label = new QLabel(QString::fromStdString(config.mounting_interface));
    mount_label->setStyleSheet("QLabel { color: #666; }");
    compatibility_layout_->addWidget(mount_label);
  }
  
  // Compatible robot brands
  if (!config.compatible_robot_brands.empty()) {
    QLabel* brands_header = new QLabel("Compatible Robot Brands:");
    brands_header->setStyleSheet("QLabel { font-weight: bold; color: #333; margin-top: 10px; }");
    compatibility_layout_->addWidget(brands_header);
    
    for (const auto& brand : config.compatible_robot_brands) {
      QLabel* brand_label = new QLabel("✅ " + QString::fromStdString(brand));
      brand_label->setStyleSheet("QLabel { color: #388e3c; }");
      compatibility_layout_->addWidget(brand_label);
    }
  }
  
  compatibility_layout_->addStretch();
}

void EndEffectorSpecificationWidget::updatePackageStatus(const std::vector<std::string>& required, 
                                                        const std::vector<std::string>& optional)
{
  // Clear existing package status
  QLayoutItem* item;
  while ((item = package_layout_->takeAt(0)) != nullptr) {
    delete item->widget();
    delete item;
  }
  
  auto& config_manager = EndEffectorConfigManager::getInstance();
  auto missing_packages = config_manager.getMissingPackages(current_end_effector_);
  
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

void EndEffectorSpecificationWidget::createSpecificationRow(const QString& label, const QString& value, 
                                                           QGridLayout* layout, int row)
{
  QLabel* label_widget = new QLabel(label);
  label_widget->setStyleSheet("QLabel { font-weight: bold; color: #333; }");
  
  QLabel* value_widget = new QLabel(value);
  value_widget->setStyleSheet("QLabel { color: #666; }");
  
  layout->addWidget(label_widget, row, 0);
  layout->addWidget(value_widget, row, 1);
}

QString EndEffectorSpecificationWidget::formatSpecValue(double value, const QString& unit)
{
  // Format numbers nicely - remove unnecessary decimal places
  if (value == static_cast<int>(value)) {
    return QString::number(static_cast<int>(value)) + " " + unit;
  } else {
    return QString::number(value, 'f', 1) + " " + unit;
  }
}

QString EndEffectorSpecificationWidget::formatActuationType(const std::string& actuation_type)
{
  if (actuation_type == "electric") return "⚡ Electric";
  else if (actuation_type == "pneumatic") return "💨 Pneumatic";
  else if (actuation_type == "hydraulic") return "🔧 Hydraulic";
  else return QString::fromStdString(actuation_type);
}

QString EndEffectorSpecificationWidget::formatInterfaceType(const std::string& interface_type)
{
  if (interface_type == "modbus") return "📡 Modbus RTU";
  else if (interface_type == "ethernet") return "🌐 Ethernet";
  else if (interface_type == "io") return "🔌 Digital I/O";
  else if (interface_type == "canbus") return "🚌 CAN Bus";
  else return QString::fromStdString(interface_type);
}

void EndEffectorSpecificationWidget::clear()
{
  end_effector_name_label_->setText("No End-Effector Selected");
  manufacturer_label_->setText("");
  end_effector_description_label_->setText("Select an end-effector to view its specifications");
  end_effector_image_label_->clear();
  end_effector_image_label_->setText("No Image");
  end_effector_image_label_->setStyleSheet("QLabel { border: 1px dashed #ccc; color: #999; }");
  
  // Clear specifications
  QLayoutItem* item;
  while ((item = specs_layout_->takeAt(0)) != nullptr) {
    delete item->widget();
    delete item;
  }
  
  // Clear capabilities
  while ((item = capabilities_layout_->takeAt(0)) != nullptr) {
    delete item->widget();
    delete item;
  }
  
  // Clear compatibility
  while ((item = compatibility_layout_->takeAt(0)) != nullptr) {
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