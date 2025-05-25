#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QWidget>
#include <rclcpp/rclcpp.hpp>
#include "robot_description_core_plugins/robot_selection_widget.hpp"
#include "robot_description_core_plugins/robot_config_manager.hpp"

int main(int argc, char** argv)
{
    // Initialize ROS2
    rclcpp::init(argc, argv);
    auto node = rclcpp::Node::make_shared("test_advanced_features");
    
    // Initialize Qt Application
    QApplication app(argc, argv);
    
    // Create main window
    QMainWindow window;
    window.setWindowTitle("Advanced Robot Selection Features Test");
    window.resize(1200, 800);
    
    // Create the enhanced robot selection widget
    auto* widget = new robot_description::core_plugins::RobotSelectionWidget();
    
    // Initialize the widget (this would normally be done by the setup assistant framework)
    // For testing, we'll create a minimal setup
    widget->onInit();
    
    window.setCentralWidget(widget);
    window.show();
    
    // Run the application
    int result = app.exec();
    
    // Cleanup
    rclcpp::shutdown();
    return result;
}