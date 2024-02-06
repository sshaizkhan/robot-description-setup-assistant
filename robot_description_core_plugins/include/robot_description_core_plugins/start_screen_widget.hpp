#pragma once

#include <robot_description_setup_framework/qt/helper_widgets.hpp>

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
class StartScreenWidget : public QWidget
{
  Q_OBJECT
public:
  explicit StartScreenWidget(QWidget* parent = nullptr);
  void onInit();

  ~StartScreenWidget();

  std::string getName() const
  {
    return "Start Screen";
  }

  void focusGiven();

  // Qt Components
  SelectModeWidget* select_mode_widget_;
  setup_framework::LoadPathArgsWidget* stack_path_;
  setup_framework::LoadPathArgsWidget* urdf_file_;
  QPushButton* btn_load_;
  QLabel* next_label_;
  QProgressBar* progess_bar_;
  QImage* right_image_;
  QLabel* right_image_label_;

private Q_SLOTS:
  // Slot Event Handlers

  void showNewOptions();

  void showExistingOptions();

  void loadFilesClicked();

  void onPackagePathChanged(const QString& package_path);

  void onUrdfPathChanged(const QString& path);

private:
  rclcpp::Node::SharedPtr node_;

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
