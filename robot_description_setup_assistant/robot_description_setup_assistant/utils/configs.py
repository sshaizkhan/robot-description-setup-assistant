import math
import os
from dataclasses import dataclass, fields

import yaml


@dataclass
class Position:
    x: float
    y: float
    z: float


@dataclass
class Orientation:
    roll: float
    pitch: float
    yaw: float


@dataclass
class KinematicComponent:
    position: Position
    orientation: Orientation


@dataclass
class Kinematics:
    shoulder: KinematicComponent
    upper_arm: KinematicComponent
    forearm: KinematicComponent
    wrist_1: KinematicComponent
    wrist_2: KinematicComponent
    wrist_3: KinematicComponent


@dataclass
class JointParameters:
    has_acceleration_limits: bool
    has_effort_limits: bool
    has_position_limits: bool
    has_velocity_limits: bool
    max_effort: float
    max_position: float
    max_velocity: float
    min_position: float


@dataclass
class JointLimits:
    shoulder: JointParameters
    shoulder_lift: JointParameters
    elbow_joint: JointParameters
    wrist_1: JointParameters
    wrist_2: JointParameters
    wrist_3: JointParameters


@dataclass
class LinksParams:
    radius: float
    length: float


@dataclass
class Links:
    base: LinksParams
    shoulder: LinksParams
    upperarm: LinksParams
    forearm: LinksParams
    wrist_1: LinksParams
    wrist_2: LinksParams
    wrist_3: LinksParams


@dataclass
class CenterOfMass:
    shoulder_cog: Position
    upper_arm_cog: Position
    forearm_cog: Position
    wrist_1_cog: Position
    wrist_2_cog: Position
    wrist_3_cog: Position


@dataclass
class DHParameters:
    d1: float
    a2: float
    a3: float
    d4: float
    d5: float
    d6: float


@dataclass
class JointOffset:
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
class PhysicalParameters:
    dh_parameters: DHParameters
    offsets: JointOffset
    inertia_parameters: IntertiaParameters


def dataclass_from_dict(klass, d):
    try:
        fieldtypes = {f.name: f.type for f in fields(klass)}
        return klass(**{f: dataclass_from_dict(fieldtypes[f], d[f]) for f in d})
    except:
        return d


def create_kinematics_from_yaml(default_kinematics_path: str) -> Kinematics:
    with open(default_kinematics_path, "r") as yaml_file:
        kinematic_config_dict = yaml.safe_load(yaml_file)

    kinematic_components = [
        KinematicComponent(
            dataclass_from_dict(Position, pose),
            dataclass_from_dict(Orientation, pose),
        )
        for kinematic in kinematic_config_dict.values()
        for pose in kinematic.values()
    ]

    kinematics = Kinematics(*kinematic_components)
    return kinematics


def create_joint_limits_from_yaml(joint_limits_config_path: str) -> JointLimits:
    with open(joint_limits_config_path, "r") as yaml_file:
        joint_limits_config_dict = yaml.safe_load(yaml_file)

    joint_params_list = [
        dataclass_from_dict(
            JointParameters,
            {
                "has_acceleration_limits": joint_params["has_acceleration_limits"],
                "has_effort_limits": joint_params["has_effort_limits"],
                "has_position_limits": joint_params["has_position_limits"],
                "has_velocity_limits": joint_params["has_velocity_limits"],
                "max_effort": joint_params["max_effort"],
                "max_position": math.radians(joint_params["max_position"]),
                "max_velocity": math.radians(joint_params["min_position"]),
                "min_position": math.radians(joint_params["max_velocity"]),
            },
        )
        for joint_data in joint_limits_config_dict.values()
        for joint_params in joint_data.values()
    ]

    joint_limits = JointLimits(*joint_params_list)

    return joint_limits


def create_physical_parameters_from_yaml(
    physical_parameters_config_path: str,
) -> PhysicalParameters:
    with open(physical_parameters_config_path, "r") as yaml_file:
        physical_parameters_config_dict = yaml.safe_load(yaml_file)

    # print(physical_parameters_config_dict)

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
    a = create_kinematics_from_yaml(default_kinematics_path)

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
    b = create_joint_limits_from_yaml(joint_limits_config_path)

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
    c = create_physical_parameters_from_yaml(physical_param_config_path)
