import yaml
import os
from dataclasses import dataclass, fields


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
class JointParameters:
    has_acceleration_limits: bool
    has_effort_limits: bool
    has_position_limits: bool
    has_velocity_limits: bool
    max_effort: float
    max_position: float
    max_velocity: float
    min_position: float


def create_dynamic_class(names):
    components = {name: f"components['{name}']" for name in names}

    class DynamicClass:
        def __init__(self, **kwargs):
            for name, value in kwargs.items():
                setattr(self, name, value)

        def __repr__(self):
            args = ", ".join([f"{name}={components[name]}" for name in names])
            return f"DynamicClass({args})"

    return DynamicClass


def dataclass_from_dict(klass, d):
    try:
        fieldtypes = {f.name: f.type for f in fields(klass)}
        return klass(**{f: dataclass_from_dict(fieldtypes[f], d[f]) for f in d})
    except:
        return d


def create_kinematics_from_yaml(default_kinematics_path: str):
    kinematic_config_dict: dict
    with open(default_kinematics_path, "r") as yaml_file:
        kinematic_config_dict = yaml.safe_load(yaml_file)

    Kinematics = create_dynamic_class(kinematic_config_dict.keys())
    print("=======================================")

    components = {}
    for name, data in kinematic_config_dict.items():
        position = dataclass_from_dict(
            Position, {"x": data["x"], "y": data["y"], "z": data["z"]}
        )

        orientation = dataclass_from_dict(
            Orientation,
            {"roll": data["roll"], "pitch": data["pitch"], "yaw": data["yaw"]},
        )

        component = KinematicComponent(position, orientation)
        components[name] = component

    # Create a Kinematics object using the components list as arguments
    kinematics = Kinematics(**components)

    return kinematics


def create_joint_limits_from_yaml(joint_limits_path: str):
    joint_config_dict: dict

    with open(joint_limits_path, "r") as yaml_file:
        joint_config_dict = yaml.safe_load(yaml_file)

    JointsClass = create_dynamic_class(joint_config_dict.keys())

    joint_names = {}
    for name, data in joint_config_dict.items():
        joint_parameters = dataclass_from_dict(
            JointParameters,
            {
                "has_acceleration_limits": data["has_acceleration_limits"],
                "has_effort_limits": data["has_effort_limits"],
                "has_position_limits": data["has_position_limits"],
                "has_velocity_limits": data["has_velocity_limits"],
                "max_effort": data["max_effort"],
                "max_position": data["max_position"],
                "max_velocity": data["max_velocity"],
                "min_position": data["min_position"],
            },
        )
        joint_names[name] = joint_parameters
    
    print(joint_parameters)

    # joints_limits = JointsClass(**joint_names)
    # print(joints_limits)

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

    joint_limits_path = os.path.abspath(
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
    create_joint_limits_from_yaml(joint_limits_path)
