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
/* Modified from Original code by Dave Coleman */

#include "robot_description_setup_framework/qt/helper_widgets.hpp"

namespace robot_description::setup_framework
{
// HeaderWidget Implementation
HeaderWidget::HeaderWidget(const std::string& title, const std::string& instructions, QWidget* parent) : QWidget(parent)
{
  configureLayout();
  setupTitle(title);
  setupDescription(instructions);
}

void HeaderWidget::setupTitle(const std::string& title)
{
  QLabel* title_label = new QLabel(QString::fromStdString(title), this);
  title_label->setFont(QFont("Arial", 18, QFont::Bold));
  title_label->setWordWrap(true);
  static_cast<QVBoxLayout*>(layout())->addWidget(title_label);
}

void HeaderWidget::setupDescription(const std::string& instructions)
{
  QLabel* instructions_label = new QLabel(QString::fromStdString(instructions), this);
  instructions_label->setWordWrap(true);
  static_cast<QVBoxLayout*>(layout())->addWidget(instructions_label);
}

void HeaderWidget::configureLayout()
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(10, 10, 10, 10);
  setLayout(layout);
}

// LoadPathWidget Implementation
LoadPathWidget::LoadPathWidget(const QString& title, const QString& instructions, QWidget* parent, bool directoryOnly,
                               bool loadOnly, bool showBrowseBtn)
  : QFrame(parent), directory_only_(directoryOnly), load_only_(loadOnly)
{
  setupWidget(title, instructions, showBrowseBtn);
}

void LoadPathWidget::setupWidget(const QString& title, const QString& instructions, bool& show_browse_btn)
{
  setFrameShape(QFrame::StyledPanel);
  setFrameShadow(QFrame::Raised);
  setLineWidth(1);

  QVBoxLayout* main_layout = new QVBoxLayout(this);
  QHBoxLayout* path_layout = new QHBoxLayout();

  QLabel* title_label = new QLabel(title, this);
  title_label->setFont(QFont("Sans Serif", 12, QFont::Bold));
  main_layout->addWidget(title_label);

  QLabel* instruction_label = new QLabel(instructions, this);
  instruction_label->setWordWrap(true);
  main_layout->addWidget(instruction_label);
  if (show_browse_btn)
  {
    path_edit_ = new QLineEdit(this);
    connect(path_edit_, &QLineEdit::textChanged, this, &LoadPathWidget::pathChanged);
    connect(path_edit_, &QLineEdit::editingFinished, this, &LoadPathWidget::pathEditingFinished);
    path_layout->addWidget(path_edit_);

    QPushButton* browse_button = new QPushButton("Browse", this);
    connect(browse_button, &QPushButton::clicked, this, &LoadPathWidget::openFileDialog);
    path_layout->addWidget(browse_button);
    main_layout->addLayout(path_layout);
  }

  QSizePolicy size_policy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
  setSizePolicy(size_policy);
}

void LoadPathWidget::openFileDialog()
{
  QString path;
  if (directory_only_)  // only allow user to select a directory
  {
    path = QFileDialog::getExistingDirectory(this, "Open Package Directory", path_edit_->text(),
                                             QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  }
  else  // only allow user to select file
  {
    QString start_path;

    start_path = path_edit_->text();

    if (load_only_)
    {
      path = QFileDialog::getOpenFileName(this, "Open File", start_path, "");
    }
    else
    {
      path = QFileDialog::getSaveFileName(this, "Create/Load File", start_path, "");
    }
  }

  // check they did not press cancel
  if (!path.isNull())
    path_edit_->setText(path);
}

QString LoadPathWidget::getPath() const
{
  return path_edit_->text();
}

void LoadPathWidget::setPath(const QString& path)
{
  path_edit_->setText(path);
}

void LoadPathWidget::setPath(const std::string& path)
{
  path_edit_->setText(QString::fromStdString(path));
}

// LoadPathArgsWidget Implementation
LoadPathArgsWidget::LoadPathArgsWidget(const QString& title, const QString& instructions,
                                       const QString& /*argInstructions*/, QWidget* parent, bool directoryOnly,
                                       bool loadOnly, bool showBrowseBtn)
  : LoadPathWidget(title, instructions, parent, directoryOnly, loadOnly, showBrowseBtn)
{
  // arguments_label_ = new QLabel(argInstructions, this);
  // layout()->addWidget(arguments_label_);

  // arguments_edit_ = new QLineEdit(this);
  // layout()->addWidget(arguments_edit_);
}

QString LoadPathArgsWidget::getArguments() const
{
  return arguments_edit_->text();
}

void LoadPathArgsWidget::setArguments(const QString& args)
{
  arguments_edit_->setText(args);
}

void LoadPathArgsWidget::setArgumentsEnabled(bool enabled)
{
  arguments_edit_->setEnabled(enabled);
}

AddInfoWidget::AddInfoWidget(const std::string& title, const std::string& instructions, QWidget* parent)
  : QFrame(parent)
{
  setupWidget(QString::fromStdString(title), QString::fromStdString(instructions));
}

void AddInfoWidget::setupWidget(const QString& title, const QString& description)
{
  setFrameShape(QFrame::StyledPanel);
  setFrameShadow(QFrame::Raised);
  setLineWidth(1);

  QVBoxLayout* main_layout = new QVBoxLayout(this);

  QLabel* title_label = new QLabel(title, this);
  title_label->setFont(QFont("Sans Serif", 12, QFont::Bold));
  main_layout->addWidget(title_label);

  QLabel* description_label = new QLabel(description, this);
  description_label->setWordWrap(true);
  main_layout->addWidget(description_label);

  QSizePolicy size_policy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
  setSizePolicy(size_policy);
}

}  // namespace robot_description::setup_framework
