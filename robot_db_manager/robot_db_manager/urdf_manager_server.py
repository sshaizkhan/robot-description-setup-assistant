#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from urdf_manager_interfaces.srv import ReadURDF
from urdf_manager_interfaces.msg import XacroArgs

from ament_index_python.packages import get_package_share_directory

import xacro
import os


class URDFServiceNode(Node):
    def __init__(self):
        super().__init__('urdf_service_node')
        self.service = self.create_service(
            ReadURDF, 'read_urdf', self.read_urdf_callback)

        self.ur_description_pkg_path = get_package_share_directory(
            'ur_description')
        self.urdf_file = os.path.join(
            self.ur_description_pkg_path, 'urdf', 'ur.urdf.xacro')
        self.get_logger().info('URDF Service has been started.')

    def process_xacro_to_urdf(self, xacro_args={}):

        if not os.path.exists(self.urdf_file):
            self.get_logger().error(
                'URDF file not found at: ' + self.urdf_file)
            return ""
        doc = xacro.process_file(self.urdf_file, mappings=xacro_args)
        robot_desc = doc.toprettyxml(indent='  ')

        # return a string
        return robot_desc

    def read_urdf_callback(self, request: ReadURDF.Request, response: ReadURDF.Response):
        # Update with the correct path
        self.get_logger().info('URDF file requested.')

        xacro_args = {pair.key: pair.value for pair in request.xacro_args}

        response.urdf_content = self.process_xacro_to_urdf(xacro_args)

        self.get_logger().info(
            f"URDF file sent: {response.urdf_content[:100]}...")

        self._logger.info('URDF file sent.')
        return response


def main(args=None):
    rclpy.init(args=args)
    urdf_service_node = URDFServiceNode()
    rclpy.spin(urdf_service_node)
    rclpy.shutdown()


if __name__ == '__main__':
    main()
