import math
import os
from dataclasses import dataclass, fields
from typing import Optional

import yaml


@dataclass
class Position:
    """Represents a point in 3D space"""

    x: float  # the x coordinate
    y: float  # the y coordinate
    z: float  # the z coordinate

    def __post_init__(self):
        if not isinstance(self.x, (int, float)):
            raise TypeError(
                f"x coordinate must be numeric, got {type(self.x).__name__}"
            )
        if not isinstance(self.y, (int, float)):
            raise TypeError(
                f"y coordinate must be numeric, got {type(self.y).__name__}"
            )
        if not isinstance(self.z, (int, float)):
            raise TypeError(
                f"z coordinate must be numeric, got {type(self.z).__name__}"
            )


@dataclass
class Orientation:
    """Represents an orientation in 3D space"""

    roll: float  # rotation about the x-axis
    pitch: float  # rotation about the y-axis
    yaw: float  # rotation about the z-axis

    def __post_init__(self):
        if not isinstance(self.roll, (int, float)):
            raise TypeError(
                f"roll coordinate must be numeric, got {type(self.roll).__name__}"
            )
        if not isinstance(self.pitch, (int, float)):
            raise TypeError(
                f"pitch coordinate must be numeric, got {type(self.pitch).__name__}"
            )
        if not isinstance(self.yaw, (int, float)):
            raise TypeError(
                f"yaw coordinate must be numeric, got {type(self.yaw).__name__}"
            )


@dataclass
class KinematicComponent:
    """Represents a component of the robot's kinematics"""

    position: Position  # the component's position in 3D space
    orientation: Orientation  # the component's orientation in 3D space

    def __eq__(self, other):
        """Overrides the default equality comparison for KinematicComponent instances."""
        if isinstance(other, KinematicComponent):
            return (
                self.position == other.position
                and self.orientation == other.orientation
            )
        return NotImplemented

    def __post_init__(self):
        """Performs additional initialization after the instance has been created."""
        if not isinstance(self.position, Position):
            raise TypeError("position must be an instance of Position")
        if not isinstance(self.orientation, Orientation):
            raise TypeError("orientation must be an instance of Orientation")

    def __repr__(self):
        """Returns a string representation of the instance."""
        return f"{self.__class__.__name__}(position={self.position}, orientation={self.orientation})"


@dataclass
class Kinematics:
    """Represents the kinematics of a robot"""

    shoulder: KinematicComponent  # the kinematic component for the shoulder joint
    upper_arm: KinematicComponent  # the kinematic component for the upper arm
    forearm: KinematicComponent  # the kinematic component for the forearm
    wrist_1: KinematicComponent  # the kinematic component for the first wrist joint
    wrist_2: KinematicComponent  # the kinematic component for the second wrist joint
    wrist_3: KinematicComponent  # the kinematic component for the third wrist joint

    def __eq__(self, other):
        if not isinstance(other, Kinematics):
            return False
        return all(
            getattr(self, attr) == getattr(other, attr)
            for attr in [
                "shoulder",
                "upper_arm",
                "forearm",
                "wrist_1",
                "wrist_2",
                "wrist_3",
            ]
        )

    def __post_init__(self):
        for attr in [
            "shoulder",
            "upper_arm",
            "forearm",
            "wrist_1",
            "wrist_2",
            "wrist_3",
        ]:
            if not isinstance(getattr(self, attr), KinematicComponent):
                raise TypeError(
                    f"Attribute '{attr}' must be of type KinematicComponent"
                )

    def __repr__(self):
        return (
            f"Kinematics("
            f"shoulder={self.shoulder!r}, "
            f"upper_arm={self.upper_arm!r}, "
            f"forearm={self.forearm!r}, "
            f"wrist_1={self.wrist_1!r}, "
            f"wrist_2={self.wrist_2!r}, "
            f"wrist_3={self.wrist_3!r})"
        )


@dataclass
class JointParameters:
    """
    A data class representing the parameters of a joint in a robotic arm.

    Attributes:
        has_acceleration_limits (bool): True if the joint has acceleration limits, False otherwise.
        has_effort_limits (bool): True if the joint has effort limits, False otherwise.
        has_position_limits (bool): True if the joint has position limits, False otherwise.
        has_velocity_limits (bool): True if the joint has velocity limits, False otherwise.
        max_effort (float): The maximum effort the joint can exert.
        max_position (float): The maximum position the joint can reach, in radians.
        max_velocity (float): The maximum velocity the joint can achieve, in radians per second.
        min_position (float): The minimum position the joint can reach, in radians.
    """

    has_acceleration_limits: bool
    has_effort_limits: bool
    has_position_limits: bool
    has_velocity_limits: bool
    max_effort: float
    max_position: float
    max_velocity: float
    min_position: float

    def __post_init__(self):
        """
        Converts the values of `max_position`, `max_velocity`, and `min_position` to radians.
        """
        self.max_position = math.radians(self.max_position)
        self.max_velocity = math.radians(self.max_velocity)
        self.min_position = math.radians(self.min_position)

    def __eq__(self, other):
        """
        Compares two JointParameters objects for equality.

        Args:
            other (JointParameters): The other JointParameters object.

        Returns:
            bool: True if the two JointParameters objects are equal, False otherwise.
        """
        if not isinstance(other, JointParameters):
            return False

        return (
            self.has_acceleration_limits == other.has_acceleration_limits
            and self.has_effort_limits == other.has_effort_limits
            and self.has_position_limits == other.has_position_limits
            and self.has_velocity_limits == other.has_velocity_limits
            and math.isclose(self.max_effort, other.max_effort)
            and math.isclose(self.max_position, other.max_position)
            and math.isclose(self.max_velocity, other.max_velocity)
            and math.isclose(self.min_position, other.min_position)
        )

    def validate(self):
        """
        Validates the JointParameters object.

        Raises:
            ValueError: If the JointParameters object is invalid.
        """
        if self.has_position_limits and self.max_position <= self.min_position:
            raise ValueError("max_position must be greater than min_position")

        if self.has_velocity_limits and self.max_velocity <= 0:
            raise ValueError("max_velocity must be greater than 0")

        if self.has_effort_limits and self.max_effort <= 0:
            raise ValueError("max_effort must be greater than 0")


@dataclass
class JointLimits:
    """
    Contains the limit parameters for each joint in the robotic arm.

    Attributes:
    -----------
    shoulder_pan: JointParameters
        Joint parameters for the shoulder_pan joint.
    shoulder_lift: JointParameters
        Joint parameters for the shoulder_lift joint.
    elbow_joint: JointParameters
        Joint parameters for the elbow_joint joint.
    wrist_1: JointParameters
        Joint parameters for the wrist_1 joint.
    wrist_2: JointParameters
        Joint parameters for the wrist_2 joint.
    wrist_3: JointParameters
        Joint parameters for the wrist_3 joint.
    """

    shoulder_pan: JointParameters
    shoulder_lift: JointParameters
    elbow_joint: JointParameters
    wrist_1: JointParameters
    wrist_2: JointParameters
    wrist_3: JointParameters

    def validate(self):
        """
        Validates the JointLimits object.

        Raises:
            ValueError: If any of the JointParameters objects are invalid.
        """
        for joint in fields(self):
            joint_param: JointParameters = getattr(self, joint.name)
            joint_param.validate()


@dataclass
class LinksParams:
    """
    Contains the geometric parameters of a single link in the robotic arm.

    Attributes:
    -----------
    radius: float
        Radius of the link.
    length: float
        Length of the link.
    """

    radius: float
    length: float

    def __str__(self):
        return f"LinksParams(radius={self.radius}, length={self.length})"

    def __repr__(self):
        return f"LinksParams(radius={self.radius}, length={self.length})"

    def __eq__(self, other):
        if not isinstance(other, LinksParams):
            return False
        return self.radius == other.radius and self.length == other.length

    def __getattr__(self, name):
        raise AttributeError(f"'LinksParams' object has no attribute '{name}'")

    def __setattr__(self, name, value):
        if name not in ["radius", "length"]:
            raise AttributeError(f"'LinksParams' object has no attribute '{name}'")
        object.__setattr__(self, name, value)

    def validate(
        self, radius: Optional[float] = None, length: Optional[float] = None
    ) -> None:
        if radius is not None and radius <= 0:
            raise ValueError("Radius must be a positive number.")
        if length is not None and length <= 0:
            raise ValueError("Length must be a positive number.")
        if radius is None:
            radius = self.radius
        if length is None:
            length = self.length
        self.radius = radius
        self.length = length


@dataclass
class Links:
    """
    Contains the geometric parameters for all the links in the robotic arm.

    Attributes:
    -----------
    base: LinksParams
        Geometric parameters for the base link.
    shoulder: LinksParams
        Geometric parameters for the shoulder link.
    upperarm: LinksParams
        Geometric parameters for the upper arm link.
    forearm: LinksParams
        Geometric parameters for the forearm link.
    wrist_1: LinksParams
        Geometric parameters for the wrist_1 link.
    wrist_2: LinksParams
        Geometric parameters for the wrist_2 link.
    wrist_3: LinksParams
        Geometric parameters for the wrist_3 link.
    """

    base: LinksParams
    shoulder: LinksParams
    upperarm: LinksParams
    forearm: LinksParams
    wrist_1: LinksParams
    wrist_2: LinksParams
    wrist_3: LinksParams

    def validate(self):
        for link in fields(self):
            link.value.validate()


@dataclass
class CenterOfMass:
    """
    Contains the center of mass of each link in the robotic arm.

    Attributes:
    -----------
    shoulder_cog: Position
        Position of the center of mass of the shoulder link.
    upper_arm_cog: Position
        Position of the center of mass of the upper arm link.
    forearm_cog: Position
        Position of the center of mass of the forearm link.
    wrist_1_cog: Position
        Position of the center of mass of the wrist_1 link.
    wrist_2_cog: Position
        Position of the center of mass of the wrist_2 link.
    wrist_3_cog: Position
        Position of the center of mass of the wrist_3 link.
    """

    shoulder_cog: Position
    upper_arm_cog: Position
    forearm_cog: Position
    wrist_1_cog: Position
    wrist_2_cog: Position
    wrist_3_cog: Position


@dataclass
class DHParameters:
    """
    Contains the Denavit-Hartenberg parameters for the robotic arm.

    Attributes:
    -----------
    d1: float
        Offset from the base frame to the first joint.
    a2: float
        Distance between the first and second joints along the x-axis.
    a3: float
        Distance between the second and third joints along the x-axis.
    d4: float
        Distance between the third and fourth joints along the z-axis.
    d5: float
        Distance between the fourth and fifth joints along the x-axis.
    d6: float
        Distance between the fifth joint and the end effector along the z-axis.
    """

    d1: float
    a2: float
    a3: float
    d4: float
    d5: float
    d6: float


@dataclass
class JointOffset:
    """
    Contains the offset parameters for the shoulder and elbow joints.

    Attributes:
    -----------
    shoulder_offset: float
        Offset parameter for the shoulder joint.
    elbow_offset: float
        Offset parameter for the elbow joint.
    """

    shoulder_offset: float
    elbow_offset: float


@dataclass
class IntertiaParameters:
    base_mass: float
    shoulder_mass: float
    upper_arm_mass: float
    upper_arm_inertia_offset: float
    forearm_mass: float
    wrist_1_mass: float
    wrist_2_mass: float
    wrist_3_mass: float
    shoulder_radius: float
    upper_arm_radius: float
    elbow_radius: float
    forearm_radius: float
    wrist_radius: float
    links: Links
    center_of_mass: CenterOfMass


@dataclass
class Material:
    name: str
    color: str


@dataclass
class Visual:
    mesh: str
    material: Material


@dataclass
class Collision:
    mesh: str


@dataclass
class ModelComponenets:
    visual: Visual
    collision: Collision
    visual_offset: float


@dataclass
class VisualParameters:
    base: ModelComponenets
    shoulder: ModelComponenets
    upper_arm: ModelComponenets
    forearm: ModelComponenets
    wrist_1: ModelComponenets
    wrist_2: ModelComponenets
    wrist_3: ModelComponenets


@dataclass
class PhysicalParameters:
    dh_parameters: DHParameters
    offsets: JointOffset
    inertia_parameters: IntertiaParameters


@dataclass
class RobotArmConfig:
    default_kinematics: Kinematics
    joint_limits: JointLimits
    physical_parameters: PhysicalParameters
    visual_parameters: VisualParameters


def dataclass_from_dict(klass, d):
    try:
        fieldtypes = {f.name: f.type for f in fields(klass)}
        return klass(**{f: dataclass_from_dict(fieldtypes[f], d[f]) for f in d})
    except:
        return d


def parse_default_kinematics_config(default_kinematics_path: str) -> Kinematics:
    with open(default_kinematics_path, "r") as yaml_file:
        kinematic_config_dict = yaml.safe_load(yaml_file)

    default_kinematics = dataclass_from_dict(
        Kinematics, kinematic_config_dict["kinematics"]
    )
    return default_kinematics


def parse_joint_limits_config(joint_limits_config_path: str) -> JointLimits:
    with open(joint_limits_config_path, "r") as yaml_file:
        joint_limits_config_dict = yaml.safe_load(yaml_file)

    joint_limits = dataclass_from_dict(
        JointLimits, joint_limits_config_dict["joint_limits"]
    )
    joint_limits.validate()
    return joint_limits


def parse_physical_parameters_config(
    physical_parameters_config_path: str,
) -> PhysicalParameters:
    with open(physical_parameters_config_path, "r") as yaml_file:
        physical_parameters_config_dict = yaml.safe_load(yaml_file)

    dh_parameters = dataclass_from_dict(
        DHParameters, physical_parameters_config_dict["dh_parameters"]
    )
    offset = dataclass_from_dict(
        JointOffset, physical_parameters_config_dict["offsets"]
    )

    inertia_parameters = dataclass_from_dict(
        IntertiaParameters, physical_parameters_config_dict["inertia_parameters"]
    )

    physical_parameters = PhysicalParameters(dh_parameters, offset, inertia_parameters)
    return physical_parameters


def parse_visual_parameters_config(
    visual_param_config_path: str,
) -> VisualParameters:
    with open(visual_param_config_path, "r") as yaml_file:
        visual_parameters_config_dict = yaml.safe_load(yaml_file)

    visual_parameters = dataclass_from_dict(
        VisualParameters, visual_parameters_config_dict["mesh_files"]
    )

    return visual_parameters


if __name__ == "__main__":
    current_file_path = os.path.abspath(__file__)

    default_kinematics_path = os.path.abspath(
        os.path.join(
            current_file_path,
            "..",
            "..",
            "..",
            "..",
            "robot_description_resources",
            "config",
            "ur5",
            "default_kinematics.yaml",
        )
    )
    a = parse_default_kinematics_config(default_kinematics_path)

    joint_limits_config_path = os.path.abspath(
        os.path.join(
            current_file_path,
            "..",
            "..",
            "..",
            "..",
            "robot_description_resources",
            "config",
            "ur5",
            "joint_limits.yaml",
        )
    )
    b = parse_joint_limits_config(joint_limits_config_path)

    physical_param_config_path = os.path.abspath(
        os.path.join(
            current_file_path,
            "..",
            "..",
            "..",
            "..",
            "robot_description_resources",
            "config",
            "ur5",
            "physical_parameters.yaml",
        )
    )
    c = parse_physical_parameters_config(physical_param_config_path)

    visual_param_config_path = os.path.abspath(
        os.path.join(
            current_file_path,
            "..",
            "..",
            "..",
            "..",
            "robot_description_resources",
            "config",
            "ur5",
            "visual_parameters.yaml",
        )
    )
    d = parse_visual_parameters_config(visual_param_config_path)

    robot_arm_config = RobotArmConfig(a, b, c, d)

    print(robot_arm_config.joint_limits.shoulder_pan.max_position)
