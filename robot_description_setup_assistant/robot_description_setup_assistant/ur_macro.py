import math
from lxml import etree
from typing import Tuple, List, Optional
from dataclasses import dataclass
from robot_description_setup_assistant.ur_common import RobotModelData


@dataclass
class Origin:
    xyz: Tuple[str, str, str]
    rpy: Tuple[str, str, str]


@dataclass
class JointLimits:
    lower_limit: str
    upper_limit: str
    effort_limit: str
    velocity_limit: str


@dataclass
class SafetyParams:
    safety_pos_margin: str
    safety_k_position: str


class CylinderInertia:
    def __init__(self, inertial_radius,
                 inertial_length,
                 inertial_mass,
                 inertial_origin: Origin
                 ):

        self.inertial_radius = float(inertial_radius)
        self.inertial_length = float(inertial_length)
        self.inertial_mass = float(inertial_mass)
        self.inertial_origin = inertial_origin

    def calculate_inertia(self):
        # Calculate inertia tensor components for a cylinder
        ixx_iyy = 0.0833333 * self.inertial_mass * \
            (3 * self.inertial_radius**2 + self.inertial_length**2)
        izz = 0.5 * self.inertial_mass * self.inertial_radius**2

        return {
            "ixx": ixx_iyy,
            "ixy": 0.0,
            "ixz": 0.0,
            "iyy": ixx_iyy,
            "iyz": 0.0,
            "izz": izz
        }

    def to_xml(self):
        # Calculate inertia
        inertia = self.calculate_inertia()

        # Create XML representation
        inertial_elem = etree.Element("inertial")
        etree.SubElement(inertial_elem, "mass", value=str(self.inertial_mass))
        etree.SubElement(inertial_elem, "origin",
                         xyz=" ".join(map(str, self.inertial_origin.xyz)),
                         rpy=" ".join(map(str, self.inertial_origin.rpy))
                         )
        etree.SubElement(inertial_elem, "inertia",
                         ixx="{:.9f}".format(inertia["ixx"]),
                         ixy="{:.1f}".format(inertia["ixy"]),
                         ixz="{:.1f}".format(inertia["ixz"]),
                         iyy="{:.9f}".format(inertia["iyy"]),
                         iyz="{:.1f}".format(inertia["iyz"]),
                         izz="{:.9f}".format(inertia["izz"]))

        return inertial_elem


class Link:
    def __init__(self, name: str,
                 visual_origin: Optional[Origin] = None,
                 visual_mesh: Optional[str] = None,
                 material_name: Optional[str] = None,
                 material_color: Optional[str] = None,
                 collision_origin: Optional[Origin] = None,
                 collision_mesh: Optional[str] = None,
                 inertial_origin: Optional[Origin] = None,
                 inertial_radius: Optional[str] = None,
                 inertial_length: Optional[str] = None,
                 inertial_mass: Optional[str] = None,
                 geometry_type: str = "mesh",
                 geometry_dimensions: List[str] = None,
                 ):

        self.link_name = name

        # Tuple (x, y, z), Tuple(roll, pitch, yaw)
        self.visual_origin = visual_origin
        self.visual_mesh = visual_mesh
        self.material_name = material_name
        self.material_color = material_color

        self.collision_origin = collision_origin
        self.collision_mesh = collision_mesh

        self.inertial_origin = inertial_origin
        self.inertial_radius = inertial_radius
        self.inertial_length = inertial_length
        self.inertial_mass = inertial_mass
        self.geometry_type = geometry_type
        self.geometry_dimensions = geometry_dimensions

    def to_xml(self):
        link: list = etree.Element("link", name=self.link_name)

        # Visual element
        if self.visual_origin:
            visual = etree.SubElement(link, "visual")
            etree.SubElement(visual, "origin",
                             xyz=" ".join(map(str, self.visual_origin.xyz)),
                             rpy=" ".join(map(str, self.visual_origin.rpy))
                             )
            geometry_visual = etree.SubElement(visual, "geometry")
            if self.geometry_type == "mesh":
                etree.SubElement(geometry_visual, "mesh",
                                 filename=self.visual_mesh)
            elif self.geometry_type == "cylinder":
                etree.SubElement(geometry_visual, "cylinder",
                                 radius=self.geometry_dimensions[0],
                                 length=self.geometry_dimensions[1])
            elif self.geometry_type == "box":
                etree.SubElement(geometry_visual, "box",
                                 size=" ".join(self.geometry_dimensions))
        # Material element
        if self.material_color and self.material_name:
            material = etree.SubElement(
                visual, "material", name=self.material_name)
            etree.SubElement(material, "color",
                             rgba=self.material_color)

        # Collision element
        if self.collision_origin:
            collision = etree.SubElement(link, "collision")
            etree.SubElement(collision, "origin",
                             xyz=" ".join(map(str, self.collision_origin.xyz)),
                             rpy=" ".join(map(str, self.collision_origin.rpy))
                             )
            geometry_collision = etree.SubElement(collision, "geometry")
            if self.geometry_type == "mesh":
                etree.SubElement(geometry_collision, "mesh",
                                 filename=self.collision_mesh)
            elif self.geometry_type == "cylinder":
                etree.SubElement(geometry_collision, "cylinder",
                                 radius=self.geometry_dimensions[0],
                                 length=self.geometry_dimensions[1])
            elif self.geometry_type == "box":
                etree.SubElement(geometry_collision, "box",
                                 size=" ".join(self.geometry_dimensions))

        # Inertial element
        if self.inertial_origin:
            cylindrical_inertia = CylinderInertia(
                self.inertial_radius, self.inertial_length, self.inertial_mass, self.inertial_origin)
            link.append(cylindrical_inertia.to_xml())

        return link


class Joint:
    def __init__(self,
                 joint_name: str,
                 parent_link: str,
                 child_link: str,
                 joint_origin: Origin,
                 joint_axis: Tuple[str, str, str] = (
                     "0", "0", "1"),  # Default axis for fixed joints
                 limits: JointLimits = None,  # Optional for fixed joints
                 safety_limits=False,
                 safety_params: SafetyParams = None,
                 joint_type: str = "revolute"
                 ):

        self.joint_name = joint_name
        self.parent_link = parent_link
        self.child_link = child_link
        self.joint_origin = joint_origin
        self.joint_axis = joint_axis
        self.limits = limits
        self.safety_limits = safety_limits
        self.joint_safety_params = safety_params
        self.joint_type = joint_type

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

        if self.joint_type == "revolute":
            etree.SubElement(joint, "axis",
                             xyz=" ".join(map(str, self.joint_axis))
                             )

            limit_attributes = {
                "lower": str(self.degrees_to_radian(self.limits.lower_limit)),
                "upper": str(self.degrees_to_radian(self.limits.upper_limit)),
                "effort": str(self.limits.effort_limit),
                "velocity": str(self.degrees_to_radian(self.limits.velocity_limit))
            }
            etree.SubElement(joint, "limit", **limit_attributes)

            if self.safety_limits:
                soft_lower_limit = str(
                    self.degrees_to_radian(self.limits.lower_limit) +
                    float(self.joint_safety_params.safety_pos_margin)
                )
                soft_upper_limit = str(
                    self.degrees_to_radian(self.limits.upper_limit) -
                    float(self.joint_safety_params.safety_pos_margin)
                )
                k_position = str(self.joint_safety_params.safety_k_position)

                etree.SubElement(joint, "safety_controller",
                                 soft_lower_limit=soft_lower_limit,
                                 soft_upper_limit=soft_upper_limit,
                                 k_position=k_position,
                                 k_velocity="0.0")

            etree.SubElement(joint, "dynamics", damping="0", friction="0")

        return joint


class URRobot(RobotModelData):

    def __init__(self, robot_name: str,
                 prefix: str,
                 joint_limits_parameters_file: str,
                 kinematics_parameters_file: str,
                 physical_parameters_file: str,
                 visual_parameters_file: str,
                 safety_limits=False,
                 safety_pos_margin=0.15,
                 safety_k_position=20.0
                 ):

        super().__init__(
            joint_limits_parameters_file,
            kinematics_parameters_file,
            physical_parameters_file,
            visual_parameters_file
        )

        self.robot_name = robot_name
        self.prefix = prefix
        self.safety_limits = safety_limits
        self.safety_pos_margin = safety_pos_margin
        self.safety_k_position = safety_k_position

        self.links: List[Link] = []
        self.joints: List[Joint] = []

    def create_link(self, link_name: str,
                    visual_origin: Optional[Origin] = None,
                    visual_mesh: Optional[str] = None,
                    visual_material_name: Optional[str] = None,
                    visual_material_color: Optional[str] = None,
                    collision_origin: Optional[Origin] = None,
                    collision_mesh: Optional[str] = None,
                    inertial_origin: Optional[Origin] = None,
                    inertial_radius: Optional[str] = None,
                    inertial_length: Optional[str] = None,
                    link_mass: Optional[str] = None,
                    geometry_type: str = "mesh",
                    geometry_dimensions: List[str] = None,
                    ):

        self.links.append(
            Link(
                f"{self.prefix}{link_name}",
                visual_origin,
                visual_mesh,
                visual_material_name,
                visual_material_color,
                collision_origin,
                collision_mesh,
                inertial_origin,
                inertial_radius,
                inertial_length,
                link_mass,
                geometry_type,
                geometry_dimensions
            )
        )

    def create_joint(self,
                     joint_name: str,
                     parent_link: str,
                     child_link: str,
                     joint_origin: Origin,
                     joint_axis: Tuple[str, str, str] = (
                         "0", "0", "1"),
                     joint_limits: JointLimits = None,
                     joint_type: str = "revolute"
                     ):

        safety_params = None
        if self.safety_limits:
            safety_params = SafetyParams(
                self.safety_pos_margin, self.safety_k_position
            )

        self.joints.append(
            Joint(
                f"{self.prefix}{joint_name}",
                f"{self.prefix}{parent_link}",
                f"{self.prefix}{child_link}",
                joint_origin,
                joint_axis,
                joint_limits,
                self.safety_limits,
                safety_params,
                joint_type
            )
        )

    def to_xml(self):
        self.robot: list = etree.Element("robot", name=self.robot_name)
        for link in self.links:
            self.robot.append(etree.Comment(link.link_name.upper()))
            self.robot.append(link.to_xml())

        for joint in self.joints:
            self.robot.append(etree.Comment(joint.joint_name.upper()))
            self.robot.append(joint.to_xml())

        return etree.tostring(
            self.robot, xml_declaration=True,
            encoding="UTF-8", pretty_print=True).decode()

    def write_to_file(self, filename):
        xml_content = self.to_xml()
        with open(filename, 'w') as file:
            file.write(xml_content)
