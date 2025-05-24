#include "robot_description_setup_assistant/setup_assistant_widget.hpp"

#include <QApplication>
#include <QMessageBox>
#include <boost/program_options.hpp>
#include <rviz_common/ros_integration/ros_client_abstraction.hpp>
#include <signal.h>
#include <locale.h>

#include <filesystem>

#include <rclcpp/rclcpp.hpp>

static void siginthandler(int /*param*/)
{
  QApplication::quit();
}

void usage(boost::program_options::options_description& desc, int exit_code)
{
  std::cout << desc << '\n';
  exit(exit_code);
}

int main(int argc, char** argv)
{
  std::vector<std::string> remaining_args = rclcpp::remove_ros_arguments(argc, argv);
  std::vector<char*> clean_argv;
  clean_argv.reserve(remaining_args.size());
  for (const std::string& arg : remaining_args)
  {
    clean_argv.push_back(const_cast<char*>(arg.c_str()));
  }

  // Declare the supported options
  // Parse parameters
  namespace po = boost::program_options;

  // clang-format off
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Show help message")("debug,g", "Run in debug/test mode")(
      "urdf_path,u", po::value<std::filesystem::path>(), "Optional, path to URDF file in ROS package")(
      "config_pkg,c", po::value<std::string>(), "Optional, pass in existing config package to load");
  // clang-format on

  // Parse parameters
  namespace po = boost::program_options;

  po::variables_map vm;
  try
  {
    po::store(po::parse_command_line(clean_argv.size(), &clean_argv[0], desc), vm);
    po::notify(vm);

    if (vm.count("help"))
      usage(desc, 0);
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << '\n';
    usage(desc, 1);
  }

  // Start ROS Node
  std::unique_ptr<rviz_common::ros_integration::RosClientAbstraction> client = std::make_unique<rviz_common::ros_integration::RosClientAbstraction>();
  rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr node = client->init(argc, argv, "robot_description_setup_assistant", false);

  // Create Qt Application
  QApplication qt_app(argc, argv);
  // numeric values should always be POSIX
  setlocale(LC_NUMERIC, "C");

  // Load QT Widget
  std::unique_ptr<robot_description::setup_assistant::SetupRobotDescriptionAssistantWidget> setup_assistant_widget = std::make_unique<robot_description::setup_assistant::SetupRobotDescriptionAssistantWidget>(node, nullptr, vm);  setup_assistant_widget->setMinimumWidth(1290);
  setup_assistant_widget->setMinimumHeight(600);

  setup_assistant_widget->show();

  // Create main window

  signal(SIGINT, siginthandler);
  const int result = qt_app.exec();
  setup_assistant_widget.reset();
  rclcpp::shutdown();
  node.reset();
  client.reset();
  return result;
}
