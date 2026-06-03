#include <QApplication>
#include <QMainWindow>
#include <rclcpp/rclcpp.hpp>
#include "robot_description_core_plugins/robot_selection_widget.hpp"
#include "robot_description_setup_framework/app_context.hpp"

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

    // Create the enhanced robot selection widget and initialize its SetupStep
    // with an AppContext (no RViz panel in this harness -> spec/info panel only).
    auto context = std::make_shared<robot_description::AppContext>(node);
    auto* widget = new robot_description::core_plugins::RobotSelectionWidget();
    widget->initialize(node, &window, nullptr, context);

    window.setCentralWidget(widget);
    window.show();

    // Run the application
    int result = app.exec();

    // Cleanup
    rclcpp::shutdown();
    return result;
}
