import math
from lxml import etree
from typing import Tuple, Optional
import unittest

from robot_description_setup_assistant.utils.data_classes.elements import (
    Origin,
    SafetyController,
    Limits,
    Dynamics,
    JointMimic,
    Calibration,
)
from robot_description_setup_assistant.utils.data_classes.attributes import JointType


class Joint:
    def __init__(self,
                 joint_name: str,
                 parent_link: str,
                 child_link: str,
                 joint_origin: Origin,
                 joint_axis: Optional[Tuple[int, int, int]] = (1, 0, 0),
                 limits: Optional[Limits] = None,
                 safety_controller: Optional[SafetyController] = None,
                 safety_limits: Optional[bool] = False,
                 joint_type: JointType = JointType.REVOLUTE,
                 dynamics: Optional[Dynamics] = None,
                 joint_mimic: Optional[JointMimic] = None,
                 calibration: Optional[Calibration] = None,
                 ):

        self.joint_name = joint_name
        self.parent_link = parent_link
        self.child_link = child_link
        self.joint_origin = joint_origin
        self.joint_axis = joint_axis
        self.limits = limits
        self.safety_controller = safety_controller
        self.safety_limits = safety_limits
        self.joint_type = joint_type.value
        self.dynamics = dynamics
        self.joint_mimic = joint_mimic
        self.calibration = calibration

    def degrees_to_radian(self, value):
        return float(value) * math.pi / 180

    def to_xml(self):
        joint = etree.Element(
            "joint", name=self.joint_name, type=self.joint_type)
        etree.SubElement(joint, "parent", link=self.parent_link)
        etree.SubElement(joint, "child", link=self.child_link)

        etree.SubElement(joint, "origin",
                         xyz=" ".join(map(str, self.joint_origin.xyz)),
                         rpy=" ".join(map(str, self.joint_origin.rpy))
                         )

        if self.joint_type != "fixed":
            etree.SubElement(joint, "axis",
                             xyz=" ".join(map(str, self.joint_axis))
                             )
        if self.limits:
            limit_attributes = {k: str(getattr(self.limits, k)) for k in [
                "lower", "upper", "effort", "velocity"]}
            etree.SubElement(joint, "limit", **limit_attributes)

        if self.calibration:
            if self.calibration.rising:
                etree.SubElement(joint, "calibration",
                                 rising=str(self.calibration.rising))
            if self.calibration.falling:
                etree.SubElement(joint, "calibration",
                                 falling=str(self.calibration.falling))

        if self.safety_limits:
            safety_controller_attributes = {k: str(getattr(self.safety_controller, k)) for k in [
                "soft_lower_limit", "soft_upper_limit", "k_position", "k_velocity"]}
            etree.SubElement(joint, "safety_controller",
                             **safety_controller_attributes)

        if self.dynamics:
            etree.SubElement(joint, "dynamics", damping=str(
                self.dynamics.damping), friction=str(self.dynamics.friction))

        if self.joint_mimic:
            if self.joint_mimic.joint:
                etree.SubElement(joint, "mimic",
                                 joint=self.joint_mimic.joint)
            if self.joint_mimic.multiplier:
                etree.SubElement(joint, "mimic",
                                 multiplier=str(self.joint_mimic.multiplier))
            if self.joint_mimic.offset:
                etree.SubElement(joint, "mimic",
                                 offset=str(self.joint_mimic.offset))

        return joint


class TestJoint(unittest.TestCase):
    def assertXmlEquivalent(self, expected_xml_str, actual_xml_str, print_output=False):
        expected_xml = etree.fromstring(expected_xml_str)
        actual_xml = etree.fromstring(actual_xml_str)

        if print_output:
            print("Expected XML:")
            print(etree.tostring(expected_xml, pretty_print=True).decode())
            print("\nActual XML:")
            print(etree.tostring(actual_xml, pretty_print=True).decode())

        assert etree.tostring(expected_xml) == etree.tostring(actual_xml)

    def test_fixed_joint(self):
        joint = Joint(
            joint_name="panda_link0_joint",
            parent_link="panda_link0",
            child_link="panda_link1",
            joint_origin=Origin(xyz=(0, 0, 0), rpy=(0, 0, 0)),
            joint_axis=(1, 0, 0),
            joint_type=JointType.FIXED,
        )

        expected_xml = """
<joint name="panda_link0_joint" type="fixed">
  <parent link="panda_link0"/>
  <child link="panda_link1"/>
  <origin xyz="0 0 0" rpy="0 0 0"/>
</joint>
"""
        actual_xml_str = etree.tostring(
            joint.to_xml(), pretty_print=True)
        self.assertXmlEquivalent(expected_xml, actual_xml_str)

    def test_revolute_joint(self):
        joint = Joint(
            joint_name="test_joint",
            parent_link="test_link1",
            child_link="test_link2",
            joint_origin=Origin(xyz=(0, 0, 0), rpy=(0, 0, 0)),
            joint_axis=(1, 0, 0),
            limits=Limits(
                lower=-2.8973, upper=2.8973, effort=87, velocity=2.1750),
            safety_controller=SafetyController(
                soft_lower_limit=-2.0,
                soft_upper_limit=2.0,
                k_position=20.0,
                k_velocity=0.1,
            ),
            joint_type=JointType.REVOLUTE,
            dynamics=Dynamics(damping=0.0, friction=0.0),
        )

        expected_xml = """
<joint name="test_joint" type="revolute">
  <parent link="test_link1"/>
  <child link="test_link2"/>
  <origin xyz="0 0 0" rpy="0 0 0"/>
  <axis xyz="1 0 0"/>
  <limit lower="-2.8973" upper="2.8973" effort="87" velocity="2.175"/>
  <safety_controller soft_lower_limit="-2.0" soft_upper_limit="2.0" k_position="20.0" k_velocity="0.1"/>
  <dynamics damping="0.0" friction="0.0"/>
</joint>
"""
        actual_xml_str = etree.tostring(
            joint.to_xml(), pretty_print=True)
        self.assertXmlEquivalent(
            expected_xml, actual_xml_str, print_output=False)

    def test_prismatic_joint(self):
        joint = Joint(
            joint_name="finger_joint",
            parent_link="panda_hand",
            child_link="panda_leftfinger",
            joint_origin=Origin(xyz=(0, 0, 0.0584), rpy=(0, 0, 0)),
            joint_axis=(0, -1, 0),
            limits=Limits(
                lower=-1.57, upper=1.57, effort=10, velocity=1.0),
            safety_controller=SafetyController(
                soft_lower_limit=-2.0,
                soft_upper_limit=2.0,
                k_position=20.0,
                k_velocity=0.1,
            ),
            joint_type=JointType.PRISMATIC,
            joint_mimic=JointMimic(joint="panda_finger_joint1"),
        )

        expected_xml = """
<joint name="finger_joint" type="prismatic">
  <parent link="panda_hand"/>
  <child link="panda_leftfinger"/>
  <origin xyz="0 0 0.0584" rpy="0 0 0"/>
  <axis xyz="0 -1 0"/>
  <limit lower="-1.57" upper="1.57" effort="10" velocity="1.0"/>
  <safety_controller soft_lower_limit="-2.0" soft_upper_limit="2.0" k_position="20.0" k_velocity="0.1"/>
  <mimic joint="panda_finger_joint1"/>
</joint>
"""
        actual_xml_str = etree.tostring(
            joint.to_xml(), pretty_print=True)
        self.assertXmlEquivalent(
            expected_xml, actual_xml_str, print_output=False)

    def test_continuous_joint(self):
        joint = Joint(
            joint_name="panda_finger_joint1",
            parent_link="panda_leftfinger",
            child_link="panda_leftfinger_link",
            joint_origin=Origin(xyz=(0, 0, 0), rpy=(0, 0, 0)),
            joint_axis=(0, 1, 0),
            joint_type=JointType.CONTINUOUS,
        )

        expected_xml = """
<joint name="panda_finger_joint1" type="continuous">
  <parent link="panda_leftfinger"/>
  <child link="panda_leftfinger_link"/>
  <origin xyz="0 0 0" rpy="0 0 0"/>
  <axis xyz="0 1 0"/>
</joint>
"""
        actual_xml_str = etree.tostring(
            joint.to_xml(), pretty_print=True)
        self.assertXmlEquivalent(
            expected_xml, actual_xml_str, print_output=False)


if __name__ == "__main__":
    unittest.main()
