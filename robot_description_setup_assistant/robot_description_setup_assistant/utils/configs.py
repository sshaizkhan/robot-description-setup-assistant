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