import os
from robot_description_setup_assistant.end_effectors.panda_handa_xacro import PandaHandXacro
from robot_description_setup_assistant.ur_arms.common.ur5_macro import UR5Robot

from lxml import etree
from typing import Union

from robot_description_setup_assistant.utils.joint import Joint
from robot_description_setup_assistant.utils.link import Link

from robot_description_setup_assistant.utils.data_classes.attributes import JointType
from robot_description_setup_assistant.utils.data_classes.elements import Origin

from ament_index_python.packages import get_package_share_directory


def write_to_file(filename):
    xml_content = ur5_robot_xacro
    with open(filename, 'w') as file:
        file.write(xml_content)


if __name__ == "__main__":

    ur5_robot_xacro: list = etree.Element("robot", name="ur5_robot")

    shared_package_directory = get_package_share_directory(
        "robot_description_resources")
    joint_limits_parameters_file = os.path.join(
        shared_package_directory, "config", "ur5", "joint_limits.yaml")
    kinematics_parameters_file = os.path.join(
        shared_package_directory, "config", "ur5", "default_kinematics.yaml")
    physical_parameters_file = os.path.join(
        shared_package_directory, "config", "ur5", "physical_parameters.yaml")
    visual_parameters_file = os.path.join(
        shared_package_directory, "config", "ur5", "visual_parameters.yaml")

    ur5_robot = UR5Robot(
        "",
        joint_limits_parameters_file,
        kinematics_parameters_file,
        physical_parameters_file,
        visual_parameters_file,
    ).to_xml()

    panda_xacro = PandaHandXacro(
        "",
        "tool0",
        "panda",
    ).to_xml()

    world_link = Link(
        "world",
    )

    world_joint = Joint(
        "world_joint",
        joint_type=JointType.FIXED,
        parent_link="world",
        child_link="base_link",
        joint_origin=Origin(
            xyz=[0, 0, 0],
            rpy=[0, 0, 0],
        ),
    )

    combined = ur5_robot + panda_xacro + [world_link, world_joint]

    for i in combined:
        i: Union[Link, Joint]
        # xacro: bytearray = etree.tostring(i.to_xml(), pretty_print=True)
        ur5_robot_xacro.append(i.to_xml())

    ur5_robot_xacro: bytearray = etree.tostring(
        ur5_robot_xacro, xml_declaration=True, encoding="UTF-8", pretty_print=True).decode()

    write_to_file("/home/bot/rds_ws/src/robot_description_app/robot-description-setup-assistant/robot_description_setup_assistant/robot_description_setup_assistant/ur_arms/generated_xacros/ur5e_robot.xacro")
