#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from urdf_manager_interfaces.srv import ReadURDF
from urdf_manager_interfaces.msg import XacroArgs

from ament_index_python.packages import get_package_share_directory

import xacro
import os


class URDFServiceClient(Node):
    def __init__(self):
        super().__init__('urdf_service_client')
        self.client = self.create_client(ReadURDF, 'read_urdf')
        while not self.client.wait_for_service(timeout_sec=1.0):
            self.get_logger().info('Waiting for service to become available...')

    def send_request(self, xacro_args_list):
        request = ReadURDF.Request()
        request.xacro_args = xacro_args_list
        self.future = self.client.call_async(request)


def main(args=None):
    rclpy.init(args=args)

    client = URDFServiceClient()

    # Prepare the XacroArgs list for the service request
    xacro_args_list = []
    arg1 = XacroArgs()
    arg1.key = "name"
    arg1.value = "ur5_robot"
    xacro_args_list.append(arg1)

    arg2 = XacroArgs()
    arg2.key = "ur_type"
    arg2.value = "ur5"
    xacro_args_list.append(arg2)

    client.send_request(xacro_args_list)

    # Spin and wait for the service response
    while rclpy.ok():
        rclpy.spin_once(client)
        if client.future.done():
            try:
                response = client.future.result()
                print(f'Response: {response.urdf_content}')
            except Exception as e:
                client.get_logger().error('Service call failed %r' % (e,))
            break

    client.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
