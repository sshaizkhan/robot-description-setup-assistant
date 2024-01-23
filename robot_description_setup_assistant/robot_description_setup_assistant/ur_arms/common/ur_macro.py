import math
import os

from typing import Union
from lxml import etree

from robot_description_setup_assistant.utils.configs import RobotArmConfig
from robot_description_setup_assistant.utils.data_classes.attributes import JointType
from robot_description_setup_assistant.utils.data_classes.elements import Origin, Limits, SafetyController, Dynamics
from robot_description_setup_assistant.utils.joint import Joint
from robot_description_setup_assistant.utils.link import Link

from ament_index_python.packages import get_package_share_directory


class URRobot(RobotArmConfig):

    def __init__(self, prefix: str,
                 joint_limits_parameters_file: str,
                 kinematics_parameters_file: str,
                 physical_parameters_file: str,
                 visual_parameters_file: str,
                 safety_limits: bool = False,
                 safety_pos_margin: float = 0.15,
                 safety_k_position: float = 20,
                 tool_tip_link: str = "tool0",
                 ):
        super().__init__(
            kinematics_parameters_file,
            joint_limits_parameters_file,
            physical_parameters_file,
            visual_parameters_file,
        )
        self.prefix = prefix
        self.safety_limits = safety_limits
        self.safety_pos_margin = safety_pos_margin
        self.safety_k_position = safety_k_position
        self.tool_tip_link = tool_tip_link

        self.ur_robot: list = []

    @staticmethod
    def degrees_to_radian(value):
        return float(value) * math.pi / 180

    def to_xml(self):
        """ Generate xacro for UR robot link and joint."""

        # Base Link
        base_link = Link(name=f"{self.prefix}base_link",
                         visual_origin=Origin(
                             rpy=(0, 0, math.pi), xyz=(0, 0, 0)),
                         visual_mesh=self.visual_parameters.base.visual.mesh,
                         material_name=self.visual_parameters.base.visual.material.name,
                         material_color=self.visual_parameters.base.visual.material.color,
                         collision_origin=Origin(
                             rpy=(0, 0, math.pi), xyz=(0, 0, 0)),
                         collision_mesh=self.visual_parameters.base.collision.mesh,
                         inertial_origin=Origin(
                             rpy=(0, 0, 0), xyz=(0, 0, 0)),
                         inertial_radius=self.physical_parameters.inertia_parameters.links.base.radius,
                         inertial_length=self.physical_parameters.inertia_parameters.links.base.length,
                         inertial_mass=self.physical_parameters.inertia_parameters.base_mass,
                         )
        self.ur_robot.append(base_link)

        # Shoulder Link
        shoulder_link = Link(name=f"{self.prefix}shoulder_link",
                             visual_origin=Origin(
                                 rpy=(0, 0, math.pi), xyz=(0, 0, 0)),
                             visual_mesh=self.visual_parameters.shoulder.visual.mesh,
                             material_name=self.visual_parameters.shoulder.visual.material.name,
                             material_color=self.visual_parameters.shoulder.visual.material.color,
                             collision_origin=Origin(
                                 rpy=(0, 0, math.pi), xyz=(0, 0, 0)),
                             collision_mesh=self.visual_parameters.shoulder.collision.mesh,
                             inertial_origin=Origin(
                                 rpy=(0, 0, 0), xyz=(0, 0, 0)),
                             inertial_radius=self.physical_parameters.inertia_parameters.links.shoulder.radius,
                             inertial_length=self.physical_parameters.inertia_parameters.links.shoulder.length,
                             inertial_mass=self.physical_parameters.inertia_parameters.shoulder_mass,
                             )
        self.ur_robot.append(shoulder_link)

        # Shoulder Pan Joint
        shoulder_pan_joint = Joint(joint_name=f"{self.prefix}shoulder_pan_joint",
                                   joint_type=JointType.REVOLUTE,
                                   parent_link=f"{self.prefix}base_link",
                                   child_link=f"{self.prefix}shoulder_link",
                                   joint_origin=Origin(
                                       xyz=(self.default_kinematics.shoulder.position.x,
                                            self.default_kinematics.shoulder.position.y, self.default_kinematics.shoulder.position.z),
                                       rpy=(self.default_kinematics.shoulder.orientation.roll,
                                            self.default_kinematics.shoulder.orientation.pitch, self.default_kinematics.shoulder.orientation.yaw)
                                   ),
                                   joint_axis=(0, 0, 1),
                                   limits=Limits(
                                       lower=self.degrees_to_radian(
                                           self.joint_limits.shoulder_pan.min_position),
                                       upper=self.degrees_to_radian(
                                           self.joint_limits.shoulder_pan.max_position),
                                       effort=self.joint_limits.shoulder_pan.max_effort,
                                       velocity=self.degrees_to_radian(
                                           self.joint_limits.shoulder_pan.max_velocity),

                                   ),
                                   safety_limits=self.safety_limits,
                                   safety_controller=SafetyController(
                                       soft_lower_limit=self.degrees_to_radian(
                                           self.joint_limits.shoulder_pan.min_position) + self.safety_pos_margin,
                                       soft_upper_limit=self.degrees_to_radian(
                                           self.joint_limits.shoulder_pan.max_position) - self.safety_pos_margin,
                                       k_position=self.safety_k_position,
                                       k_velocity=0.0,
                                   ),
                                   dynamics=Dynamics(damping=0.0, friction=0.0)
                                   )
        self.ur_robot.append(shoulder_pan_joint)

        # Upper Arm Link
        upper_arm_link = Link(name=f"{self.prefix}upper_arm_link",
                              visual_origin=Origin(
                                  xyz=(
                                      0, 0, self.physical_parameters.offsets.shoulder_offset),
                                  rpy=(math.pi/2, 0, -1*math.pi/2)
                              ),
                              visual_mesh=self.visual_parameters.upper_arm.visual.mesh,
                              material_name=self.visual_parameters.upper_arm.visual.material.name,
                              material_color=self.visual_parameters.upper_arm.visual.material.color,
                              collision_origin=Origin(
                                  xyz=(
                                      0, 0, self.physical_parameters.offsets.shoulder_offset),
                                  rpy=(math.pi/2, 0, -1*math.pi/2)
                              ),
                              collision_mesh=self.visual_parameters.upper_arm.collision.mesh,
                              inertial_origin=Origin(
                                  xyz=(-0.5*self.physical_parameters.inertia_parameters.links.upperarm.length,
                                       0.0, self.physical_parameters.inertia_parameters.upper_arm_inertia_offset),
                                  rpy=(0, math.pi/2, 0)
                              ),
                              inertial_radius=self.physical_parameters.inertia_parameters.links.upperarm.radius,
                              inertial_length=self.physical_parameters.inertia_parameters.links.upperarm.length,
                              inertial_mass=self.physical_parameters.inertia_parameters.upper_arm_mass,
                              )
        self.ur_robot.append(upper_arm_link)

        # Shoulder Lift Joint
        shoulder_lift_joint = Joint(joint_name=f"{self.prefix}shoulder_lift_joint",
                                    joint_type=JointType.REVOLUTE,
                                    parent_link=f"{self.prefix}shoulder_link",
                                    child_link=f"{self.prefix}upper_arm_link",
                                    joint_origin=Origin(
                                        xyz=(self.default_kinematics.upper_arm.position.x,
                                             self.default_kinematics.upper_arm.position.y, self.default_kinematics.upper_arm.position.z),
                                        rpy=(self.default_kinematics.upper_arm.orientation.roll,
                                             self.default_kinematics.upper_arm.orientation.pitch, self.default_kinematics.upper_arm.orientation.yaw)
                                    ),
                                    joint_axis=(0, 0, 1),
                                    limits=Limits(
                                        lower=self.degrees_to_radian(
                                            self.joint_limits.shoulder_lift.min_position),
                                        upper=self.degrees_to_radian(
                                            self.joint_limits.shoulder_lift.max_position),
                                        effort=self.joint_limits.shoulder_lift.max_effort,
                                        velocity=self.degrees_to_radian(
                                            self.joint_limits.shoulder_lift.max_velocity),
                                    ),
                                    safety_limits=self.safety_limits,
                                    safety_controller=SafetyController(
                                        soft_lower_limit=self.degrees_to_radian(
                                            self.joint_limits.shoulder_lift.min_position) + self.safety_pos_margin,
                                        soft_upper_limit=self.degrees_to_radian(
                                            self.joint_limits.shoulder_lift.max_position) - self.safety_pos_margin,
                                        k_position=self.safety_k_position,
                                        k_velocity=0.0,
                                    ),
                                    dynamics=Dynamics(
                                        damping=0.0, friction=0.0)
                                    )
        self.ur_robot.append(shoulder_lift_joint)

        # Forearm Link
        forearm_link = Link(name=f"{self.prefix}forearm_link",
                            visual_origin=Origin(
                                xyz=(
                                    0, 0, self.physical_parameters.offsets.elbow_offset),
                                rpy=(math.pi/2, 0, -1*math.pi/2)
                            ),
                            visual_mesh=self.visual_parameters.forearm.visual.mesh,
                            material_name=self.visual_parameters.forearm.visual.material.name,
                            material_color=self.visual_parameters.forearm.visual.material.color,
                            collision_origin=Origin(
                                xyz=(
                                    0, 0, self.physical_parameters.offsets.elbow_offset),
                                rpy=(math.pi/2, 0, -1*math.pi/2)
                            ),
                            collision_mesh=self.visual_parameters.forearm.collision.mesh,
                            inertial_origin=Origin(
                                xyz=(-0.5*self.physical_parameters.inertia_parameters.links.forearm.length,
                                     0.0, self.physical_parameters.offsets.elbow_offset),
                                rpy=(0, math.pi/2, 0)
                            ),
                            inertial_radius=self.physical_parameters.inertia_parameters.links.forearm.radius,
                            inertial_length=self.physical_parameters.inertia_parameters.links.forearm.length,
                            inertial_mass=self.physical_parameters.inertia_parameters.forearm_mass,
                            )
        self.ur_robot.append(forearm_link)

        # Elbow Joint
        elbow_joint = Joint(joint_name=f"{self.prefix}elbow_joint",
                            joint_type=JointType.REVOLUTE,
                            parent_link=f"{self.prefix}upper_arm_link",
                            child_link=f"{self.prefix}forearm_link",
                            joint_origin=Origin(
                                xyz=(self.default_kinematics.forearm.position.x,
                                     self.default_kinematics.forearm.position.y, self.default_kinematics.forearm.position.z),
                                rpy=(self.default_kinematics.forearm.orientation.roll,
                                     self.default_kinematics.forearm.orientation.pitch, self.default_kinematics.forearm.orientation.yaw)
                            ),
                            joint_axis=(0, 0, 1),
                            limits=Limits(
                                lower=self.degrees_to_radian(
                                    self.joint_limits.elbow_joint.min_position),
                                upper=self.degrees_to_radian(
                                    self.joint_limits.elbow_joint.max_position),
                                effort=self.joint_limits.elbow_joint.max_effort,
                                velocity=self.degrees_to_radian(
                                    self.joint_limits.elbow_joint.max_velocity),
                            ),
                            safety_limits=self.safety_limits,
                            safety_controller=SafetyController(
                                soft_lower_limit=self.degrees_to_radian(
                                    self.joint_limits.elbow_joint.min_position) + self.safety_pos_margin,
                                soft_upper_limit=self.degrees_to_radian(
                                    self.joint_limits.elbow_joint.max_position) - self.safety_pos_margin,
                                k_position=self.safety_k_position,
                                k_velocity=0.0,
                            ),
                            dynamics=Dynamics(damping=0.0, friction=0.0)
                            )
        self.ur_robot.append(elbow_joint)

        # Wrist 1 Link
        wrist_1_link = Link(name=f"{self.prefix}wrist_1_link",
                            visual_origin=Origin(
                                xyz=(
                                    0, 0, self.visual_parameters.wrist_1.visual_offset),
                                rpy=(math.pi/2, 0, 0)
                            ),
                            visual_mesh=self.visual_parameters.wrist_1.visual.mesh,
                            material_name=self.visual_parameters.wrist_1.visual.material.name,
                            material_color=self.visual_parameters.wrist_1.visual.material.color,
                            collision_origin=Origin(
                                xyz=(
                                    0, 0, self.visual_parameters.wrist_1.visual_offset),
                                rpy=(math.pi/2, 0, 0)
                            ),
                            collision_mesh=self.visual_parameters.wrist_1.collision.mesh,
                            inertial_origin=Origin(
                                rpy=(0, 0, 0), xyz=(0, 0, 0)),
                            inertial_radius=self.physical_parameters.inertia_parameters.links.wrist_1.radius,
                            inertial_length=self.physical_parameters.inertia_parameters.links.wrist_1.length,
                            inertial_mass=self.physical_parameters.inertia_parameters.wrist_1_mass,
                            )
        self.ur_robot.append(wrist_1_link)

        # Wrist 1 Joint
        wrist_1_joint = Joint(joint_name=f"{self.prefix}wrist_1_joint",
                              joint_type=JointType.REVOLUTE,
                              parent_link=f"{self.prefix}forearm_link",
                              child_link=f"{self.prefix}wrist_1_link",
                              joint_origin=Origin(
                                  xyz=(self.default_kinematics.wrist_1.position.x,
                                       self.default_kinematics.wrist_1.position.y, self.default_kinematics.wrist_1.position.z),
                                  rpy=(self.default_kinematics.wrist_1.orientation.roll,
                                       self.default_kinematics.wrist_1.orientation.pitch, self.default_kinematics.wrist_1.orientation.yaw)
                              ),
                              joint_axis=(0, 0, 1),
                              limits=Limits(
                                  lower=self.degrees_to_radian(
                                      self.joint_limits.wrist_1.min_position),
                                  upper=self.degrees_to_radian(
                                      self.joint_limits.wrist_1.max_position),
                                  effort=self.joint_limits.wrist_1.max_effort,
                                  velocity=self.degrees_to_radian(
                                      self.joint_limits.wrist_1.max_velocity),
                              ),
                              safety_limits=self.safety_limits,
                              safety_controller=SafetyController(
                                  soft_lower_limit=self.degrees_to_radian(
                                      self.joint_limits.wrist_1.min_position) + self.safety_pos_margin,
                                  soft_upper_limit=self.degrees_to_radian(
                                      self.joint_limits.wrist_1.max_position) - self.safety_pos_margin,
                                  k_position=self.safety_k_position,
                                  k_velocity=0.0,
                              ),
                              dynamics=Dynamics(damping=0.0, friction=0.0)
                              )
        self.ur_robot.append(wrist_1_joint)

        # Wrist 2 Link
        wrist_2_link = Link(name=f"{self.prefix}wrist_2_link",
                            visual_origin=Origin(
                                xyz=(
                                    0, 0, self.visual_parameters.wrist_2.visual_offset),
                                rpy=(0, 0, 0)
                            ),
                            visual_mesh=self.visual_parameters.wrist_2.visual.mesh,
                            material_name=self.visual_parameters.wrist_2.visual.material.name,
                            material_color=self.visual_parameters.wrist_2.visual.material.color,
                            collision_origin=Origin(
                                xyz=(
                                    0, 0, self.visual_parameters.wrist_2.visual_offset),
                                rpy=(0, 0, 0)
                            ),
                            collision_mesh=self.visual_parameters.wrist_2.collision.mesh,
                            inertial_origin=Origin(
                                rpy=(0, 0, 0), xyz=(0, 0, 0)),
                            inertial_radius=self.physical_parameters.inertia_parameters.links.wrist_2.radius,
                            inertial_length=self.physical_parameters.inertia_parameters.links.wrist_2.length,
                            inertial_mass=self.physical_parameters.inertia_parameters.wrist_2_mass,
                            )
        self.ur_robot.append(wrist_2_link)

        # Wrist 2 Joint
        wrist_2_joint = Joint(joint_name=f"{self.prefix}wrist_2_joint",
                              joint_type=JointType.REVOLUTE,
                              parent_link=f"{self.prefix}wrist_1_link",
                              child_link=f"{self.prefix}wrist_2_link",
                              joint_origin=Origin(
                                  xyz=(self.default_kinematics.wrist_2.position.x,
                                       self.default_kinematics.wrist_2.position.y, self.default_kinematics.wrist_2.position.z),
                                  rpy=(self.default_kinematics.wrist_2.orientation.roll,
                                       self.default_kinematics.wrist_2.orientation.pitch, self.default_kinematics.wrist_2.orientation.yaw)
                              ),
                              joint_axis=(0, 0, 1),
                              limits=Limits(
                                  lower=self.degrees_to_radian(
                                      self.joint_limits.wrist_2.min_position),
                                  upper=self.degrees_to_radian(
                                      self.joint_limits.wrist_2.max_position),
                                  effort=self.joint_limits.wrist_2.max_effort,
                                  velocity=self.degrees_to_radian(
                                      self.joint_limits.wrist_2.max_velocity),
                              ),
                              safety_limits=self.safety_limits,
                              safety_controller=SafetyController(
                                  soft_lower_limit=self.degrees_to_radian(
                                      self.joint_limits.wrist_2.min_position) + self.safety_pos_margin,
                                  soft_upper_limit=self.degrees_to_radian(
                                      self.joint_limits.wrist_2.max_position) - self.safety_pos_margin,
                                  k_position=self.safety_k_position,
                                  k_velocity=0.0,
                              ),
                              dynamics=Dynamics(damping=0.0, friction=0.0)
                              )
        self.ur_robot.append(wrist_2_joint)

        # Wrist 3 Link
        wrist_3_link = Link(name=f"{self.prefix}wrist_3_link",
                            visual_origin=Origin(
                                xyz=(
                                    0, 0, self.visual_parameters.wrist_3.visual_offset),
                                rpy=(math.pi/2, 0, 0)
                            ),
                            visual_mesh=self.visual_parameters.wrist_3.visual.mesh,
                            material_name=self.visual_parameters.wrist_3.visual.material.name,
                            material_color=self.visual_parameters.wrist_3.visual.material.color,
                            collision_origin=Origin(
                                xyz=(
                                    0, 0, self.visual_parameters.wrist_3.visual_offset),
                                rpy=(0, 0, math.pi/2)
                            ),
                            collision_mesh=self.visual_parameters.wrist_3.collision.mesh,
                            inertial_origin=Origin(
                                rpy=(0, 0, 0),
                                xyz=(
                                    0, 0, -0.5*self.physical_parameters.inertia_parameters.links.wrist_3.length)
                            ),
                            inertial_radius=self.physical_parameters.inertia_parameters.links.wrist_3.radius,
                            inertial_length=self.physical_parameters.inertia_parameters.links.wrist_3.length,
                            inertial_mass=self.physical_parameters.inertia_parameters.wrist_3_mass,
                            )
        self.ur_robot.append(wrist_3_link)

        # Wrist 3 Joint
        wrist_3_joint = Joint(joint_name=f"{self.prefix}wrist_3_joint",
                              joint_type=JointType.REVOLUTE,
                              parent_link=f"{self.prefix}wrist_2_link",
                              child_link=f"{self.prefix}wrist_3_link",
                              joint_origin=Origin(
                                  xyz=(self.default_kinematics.wrist_3.position.x,
                                       self.default_kinematics.wrist_3.position.y, self.default_kinematics.wrist_3.position.z),
                                  rpy=(self.default_kinematics.wrist_3.orientation.roll,
                                       self.default_kinematics.wrist_3.orientation.pitch, self.default_kinematics.wrist_3.orientation.yaw)
                              ),
                              joint_axis=(0, 0, 1),
                              limits=Limits(
                                  lower=self.degrees_to_radian(
                                      self.joint_limits.wrist_3.min_position),
                                  upper=self.degrees_to_radian(
                                      self.joint_limits.wrist_3.max_position),
                                  effort=self.joint_limits.wrist_3.max_effort,
                                  velocity=self.degrees_to_radian(
                                      self.joint_limits.wrist_3.max_velocity),
                              ),
                              safety_limits=self.safety_limits,
                              safety_controller=SafetyController(
                                  soft_lower_limit=self.degrees_to_radian(
                                      self.joint_limits.wrist_3.min_position) + self.safety_pos_margin,
                                  soft_upper_limit=self.degrees_to_radian(
                                      self.joint_limits.wrist_3.max_position) - self.safety_pos_margin,
                                  k_position=self.safety_k_position,
                                  k_velocity=0.0,
                              ),
                              dynamics=Dynamics(damping=0.0, friction=0.0)
                              )
        self.ur_robot.append(wrist_3_joint)

        # Tool0 Link with box geometry
        tool0_link = Link(name=f"{self.prefix}{self.tool_tip_link}",
                          collision_origin=Origin(
                              xyz=(0, 0, 0), rpy=(0, 0, 0)
                          ),
                          geometry_type="box",
                          geometry_dimensions=[0.01, 0.01, 0.01]
                          )
        self.ur_robot.append(tool0_link)

        # Tool0 Joint
        tool0_joint = Joint(joint_name=f"{self.prefix}tool0_joint",
                            joint_type=JointType.FIXED,
                            parent_link=f"{self.prefix}wrist_3_link",
                            child_link=f"{self.prefix}{self.tool_tip_link}",
                            joint_origin=Origin(
                                xyz=(
                                    self.default_kinematics.wrist_3.position.x, 0, 0),
                                rpy=(0, 0, 0)
                            )
                            )
        self.ur_robot.append(tool0_joint)

        return self.ur_robot


if __name__ == "__main__":

    shared_package_directory = get_package_share_directory(
        "robot_description_resources")
    joint_limits_parameters_file = os.path.join(
        shared_package_directory, "config", "ur5", "joint_limits.yaml")
    print(joint_limits_parameters_file)
    kinematics_parameters_file = os.path.join(
        shared_package_directory, "config", "ur5", "default_kinematics.yaml")
    physical_parameters_file = os.path.join(
        shared_package_directory, "config", "ur5", "physical_parameters.yaml")
    visual_parameters_file = os.path.join(
        shared_package_directory, "config", "ur5", "visual_parameters.yaml")

    ur_robot = URRobot(
        "",
        joint_limits_parameters_file,
        kinematics_parameters_file,
        physical_parameters_file,
        visual_parameters_file,
        safety_limits=True,
        safety_pos_margin=0.15,
        safety_k_position=20,
        tool_tip_link="tool0",
    )
    ur_robot.to_xml()

    for i in ur_robot.ur_robot:
        i: Union[Link, Joint]
        xacro: bytearray = etree.tostring(i.to_xml(), pretty_print=True)
        print(xacro.decode())
