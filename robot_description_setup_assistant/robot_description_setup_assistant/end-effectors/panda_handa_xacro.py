import math
from lxml import etree
from typing import Optional, Union

from robot_description_setup_assistant.utils.link import Link
from robot_description_setup_assistant.utils.joint import Joint

from robot_description_setup_assistant.utils.data_classes.elements import Origin, Limits, JointMimic
from robot_description_setup_assistant.utils.data_classes.attributes import JointType


class PandaHandXacro:
    """
    A class to generate xacro for panda hand link and joint

    ...

    Attributes

    ----------
    prefix : str
        prefix for link and joint name
    conneted_to : str
        name of the link to which the panda hand is connected
    namespace : str
        namespace for the hand
    """

    def __init__(self, prefix: str,
                 conneted_to: str,
                 namespace: str,
                 ):
        self.prefix = prefix
        self.conneted_to = conneted_to
        self.namespace = namespace

        self.panda_hand: list = []

    def to_xml(self):
        """ Generate xacro for panda hand link and joint."""

        panda_hand_link = Link(name=f"{self.prefix}{self.namespace}_hand",
                               visual_origin=Origin(
                                   rpy=(0, 0, 0), xyz=(0, 0, 0)),
                               visual_mesh="package://panda_hand/visual/finger.dae",
                               collision_mesh="package://panda_hand/collision/finger.stl",
                               collision_origin=Origin(
                                   rpy=(0, 0, 0), xyz=(0, 0, 0)),
                               )
        self.panda_hand.append(panda_hand_link)

        panda_hand_joint = Joint(joint_name=f"{self.prefix}{self.namespace}_hand_joint",
                                 parent_link=f"{self.prefix}{self.conneted_to}",
                                 child_link=f"{self.prefix}{self.namespace}_hand",
                                 joint_type=JointType.FIXED,
                                 joint_origin=Origin(
                                     rpy=(0, 0, 0), xyz=(0, 0, 0)),
                                 )
        self.panda_hand.append(panda_hand_joint)

        panda_left_finger_link = Link(name=f"{self.prefix}{self.namespace}_leftfinger",
                                      visual_origin=Origin(
                                          rpy=(0, 0, 0), xyz=(0, 0, 0)),
                                      visual_mesh="package://panda_hand/visual/finger.dae",
                                      collision_mesh="package://panda_hand/collision/finger.stl",
                                      collision_origin=Origin(
                                          rpy=(0, 0, 0), xyz=(0, 0, 0)),
                                      )
        self.panda_hand.append(panda_left_finger_link)

        panda_finger_joint1 = Joint(joint_name=f"{self.prefix}{self.namespace}_finger_joint1",
                                    parent_link=f"{self.prefix}{self.namespace}_hand",
                                    child_link=f"{self.prefix}{self.namespace}_leftfinger",
                                    joint_type=JointType.PRISMATIC,
                                    joint_origin=Origin(
                                        rpy=(0, 0, 0), xyz=(0, 0, 0.0584)),
                                    joint_axis=(0, 1, 0),
                                    limits=Limits(
                                        lower=0.04, upper=0.1, effort=100, velocity=0.5)
                                    )
        self.panda_hand.append(panda_finger_joint1)

        panda_right_finger_link = Link(name=f"{self.prefix}{self.namespace}_rightfinger",
                                       visual_origin=Origin(
                                           rpy=(0, 0, math.pi/2), xyz=(0, 0, 0)),
                                       visual_mesh="package://panda_hand/visual/finger.dae",
                                       collision_mesh="package://panda_hand/collision/finger.stl",
                                       collision_origin=Origin(
                                           rpy=(0, 0, math.pi/2), xyz=(0, 0, 0)),
                                       )
        self.panda_hand.append(panda_right_finger_link)

        panda_finger_joint2 = Joint(joint_name=f"{self.prefix}{self.namespace}_finger_joint2",
                                    parent_link=f"{self.prefix}{self.namespace}_hand",
                                    child_link=f"{self.prefix}{self.namespace}_rightfinger",
                                    joint_type=JointType.PRISMATIC,
                                    joint_origin=Origin(
                                        rpy=(0, 0, 0), xyz=(0, 0, 0.0584)),
                                    joint_axis=(0, -1, 0),
                                    limits=Limits(
                                        lower=0.0, upper=0.04, effort=20, velocity=0.2),
                                    joint_mimic=JointMimic(
                                        joint=f"{self.prefix}{self.namespace}_finger_joint1"
                                    )
                                    )
        self.panda_hand.append(panda_finger_joint2)

        return self.panda_hand


if __name__ == "__main__":
    # Example usage
    panda_hand = PandaHandXacro(prefix="",
                                conneted_to="tool0",
                                namespace="panda"
                                )
    panda_hand.to_xml()

    for i in panda_hand.panda_hand:
        i: Optional[Union[Link, Joint]]
        xacro: bytearray = etree.tostring(i.to_xml(), pretty_print=True)
        print(xacro.decode())
