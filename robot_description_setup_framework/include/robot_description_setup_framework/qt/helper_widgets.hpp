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

#pragma once

#include <QWidget>
#include <QFrame>
#include <QFileDialog>
#include <QFont>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

class QLabel;
class QLineEdit;

namespace robot_description::setup_framework
{
class HeaderWidget : public QWidget
{
  Q_OBJECT
public:
  HeaderWidget(const std::string& header_title, const std::string& header_description, QWidget* parent = nullptr);

private:
  void setupTitle(const std::string& title);
  void setupDescription(const std::string& instructions);
  void configureLayout();
};

/**
 * @brief The LoadPathWidget class represents a widget for loading a file or directory path.
 *
 * This class provides a user interface for selecting a file or directory path. It inherits from QFrame.
 * The widget displays a title and instructions, and allows the user to browse and select a path.
 *
 * Signals:
 * - pathChanged(const QString& path): emitted when the selected path is changed.
 * - pathEditingFinished(): emitted when the editing of the path is finished.
 *
 * Slots:
 * - openFileDialog(): opens a file dialog to select a path.
 *
 * Public Functions:
 * - LoadPathWidget(const QString& title, const QString& instructions, QWidget* parent = nullptr,
 *                  bool directoryOnly = false, bool loadOnly = false): constructor.
 * - QString getPath() const: returns the currently selected path.
 * - void setPath(const QString& path): sets the selected path.
 * - void setPath(const std::string& path): sets the selected path from a std::string.
 */
class LoadPathWidget : public QFrame
{
  Q_OBJECT

public:
  LoadPathWidget(const LoadPathWidget&) = delete;
  LoadPathWidget(LoadPathWidget&&) = delete;
  LoadPathWidget& operator=(const LoadPathWidget&) = delete;
  LoadPathWidget& operator=(LoadPathWidget&&) = delete;
  explicit LoadPathWidget(const QString& title, const QString& description, QWidget* parent = nullptr,
                          bool directory_only = false, bool load_only = false);

  /**
   * @brief Returns the path.
   *
   * @return The path as a QString.
   */
  QString getPath() const;
  /**
   * @brief Sets the path for the widget.
   *
   * @param path The path to be set.
   */
  void setPath(const QString& path);
  /**
   * @brief Sets the path for the helper widget.
   *
   * This function sets the path for the helper widget. The path is specified as a string.
   *
   * @param path The path to be set for the helper widget.
   */
  void setPath(const std::string& path);

Q_SIGNALS:
  /**
   * @brief This function is called when the path is changed.
   *
   * @param path The new path.
   */
  void pathChanged(const QString& path);
  /**
   * @brief This function is called when the path editing is finished.
   */
  void pathEditingFinished();

private Q_SLOTS:
  void openFileDialog();

private:
  /**
   * @brief Sets up the user interface with the specified title and instructions.
   *
   * @param title The title of the user interface.
   * @param instructions The instructions for the user interface.
   */
  void setupUI(const QString& title, const QString& instructions);

  /**
   * @brief The QLineEdit object used for editing the path.
   */
  QLineEdit* path_edit_;
  /**
   * @brief A boolean flag indicating whether the object represents a directory only.
   */
  bool directory_only_;
  /**
   * @brief Indicates whether only the robot description should be loaded.
   *
   * If `load_only_` is set to `true`, only the robot description will be loaded.
   * Otherwise, additional setup steps may be performed.
   */
  bool load_only_;
};

/**
 * @brief The LoadPathArgsWidget class is a subclass of LoadPathWidget that provides additional functionality for
 * handling arguments.
 *
 * This widget is used to load a path with optional arguments. It extends the LoadPathWidget class by adding a QLineEdit
 * for entering arguments. The arguments can be retrieved using the getArguments() method and set using the
 * setArguments() method. The setArgumentsEnabled() method can be used to enable or disable the arguments input field.
 */
class LoadPathArgsWidget : public LoadPathWidget
{
  Q_OBJECT

public:
  LoadPathArgsWidget(const LoadPathArgsWidget&) = delete;
  LoadPathArgsWidget(LoadPathArgsWidget&&) = delete;
  LoadPathArgsWidget& operator=(const LoadPathArgsWidget&) = delete;
  LoadPathArgsWidget& operator=(LoadPathArgsWidget&&) = delete;
  /**
   * @brief Constructs a LoadPathArgsWidget object with the specified title, instructions, and argument instructions.
   *
   * @param title The title of the widget.
   * @param description The instructions for selecting a path.
   * @param arg_instructions The instructions for entering arguments.
   * @param parent The parent widget.
   * @param directory_only Specifies whether only directories should be selected.
   * @param load_only Specifies whether only loading should be allowed.
   */
  explicit LoadPathArgsWidget(const QString& title, const QString& description, const QString& arg_instructions,
                              QWidget* parent = nullptr, bool directory_only = false, bool load_only = false);

  /**
   * @brief Gets the arguments entered in the widget.
   *
   * @return The arguments entered in the widget.
   */
  QString getArguments() const;

  /**
   * @brief Sets the arguments to be displayed in the widget.
   *
   * @param args The arguments to be displayed in the widget.
   */
  void setArguments(const QString& args);

  /**
   * @brief Enables or disables the arguments input field.
   *
   * @param enabled Specifies whether the arguments input field should be enabled or disabled.
   */
  void setArgumentsEnabled(bool enabled);

private:
  QLineEdit* arguments_edit_; /**< The QLineEdit for entering arguments. */
  QLabel* arguments_label_;   /**< The QLabel for displaying the arguments label. */
};
}  // namespace robot_description::setup_framework
