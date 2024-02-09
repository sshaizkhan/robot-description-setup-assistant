/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2012, Willow Garage, Inc.
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
 *   * Neither the name of Willow Garage nor the names of its
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
/* Modified from original code by Dave Coleman */

#pragma once

#include <robot_description_setup_framework/qt/helper_widgets.hpp>
#include <robot_description_setup_framework/qt/setup_step_widget.hpp>

#include <QWidget>
#include <QFrame>

#include <QApplication>
#include <QFileDialog>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QString>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

#include <rclcpp/rclcpp.hpp>

#ifndef Q_MOC_RUN
#include <robot_description_core_plugins/start_screen.hpp>
#endif

// C

class QLabel;
class QProgressBar;
class QPushButton;

namespace robot_description::core_plugins
{
/**
 * @brief Represents a widget for selecting a mode.
 *
 * This widget allows the user to select a mode for the application.
 * It is used in the start screen of the robot description setup assistant.
 */
class SelectModeWidget;

/**
 * @brief The StartScreenWidget class represents a widget for the start screen of the application.
 *
 * This class inherits from QWidget and provides functionality for initializing the start screen,
 * handling focus events, and managing Qt components such as buttons, labels, and progress bars.
 * It also includes slots for event handling and private member variables for managing package settings
 * and loading files.
 */
class StartScreenWidget : public robot_description::setup_framework::SetupStepWidget
{
  Q_OBJECT
public:
  void onInit() override;

  ~StartScreenWidget();

  void focusGiven() override;

  // Qt Components
  SelectModeWidget* select_mode_widget_;
  setup_framework::LoadPathArgsWidget* stack_path_;
  // setup_framework::LoadPathArgsWidget* urdf_file_;
  setup_framework::AddInfoWidget* add_info_widget_;
  QPushButton* btn_load_;
  QLabel* next_label_;
  QProgressBar* progress_bar_;
  QImage* right_image_;
  QLabel* right_image_label_;

  SetupStep& getSetupStep() override
  {
    return setup_step_;
  }

private Q_SLOTS:
  // Slot Event Handlers

  void showNewOptions();

  void showExistingOptions();

  void loadFilesClicked();

  void onPackagePathChanged(const QString& package_path);

  void onUrdfPathChanged(const QString& path);

private:
  rclcpp::Node::SharedPtr node_;
  StartScreen setup_step_;

  bool create_new_package_;

  bool loadPackageSettings(bool show_warning = true);

  bool loadNewFiles();

  bool loadExistingFiles();
};

/**
 * @brief Widget for selecting the mode.
 */
class SelectModeWidget : public QFrame
{
  Q_OBJECT
private:
private Q_SLOTS:

public:
  /**
   * @brief Constructor for SelectModeWidget.
   * @param parent The parent widget.
   */
  SelectModeWidget(QWidget* parent = nullptr);

  QPushButton* btn_new_;        /**< Button for selecting new mode. */
  QPushButton* btn_existing_;   /**< Button for selecting existing mode. */
  QLabel* widget_instructions_; /**< Label for displaying instructions. */
};
}  // namespace robot_description::core_plugins
