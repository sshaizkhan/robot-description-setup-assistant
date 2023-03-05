import yaml
import json

from dataclasses import dataclass, asdict, fields
from typing import List, Dict


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


def dataclass_from_dict(klass, d):
    try:
        fieldtypes = {f.name: f.type for f in fields(klass)}
        return klass(**{f: dataclass_from_dict(fieldtypes[f], d[f]) for f in d})
    except:
        return d


def create_kinematics_from_yaml(default_kinematics_path: str) -> Kinematics:

    kinematic_config_dict: dict
    with open(default_kinematics_path, "r") as yaml_file:
        kinematic_config_dict = yaml.safe_load(yaml_file)

    print(kinematic_config_dict)
    print("=======================================")

    components = [
        KinematicComponent(
            dataclass_from_dict(
                Position, {"x": data["x"], "y": data["y"], "z": data["z"]}
            ),
            dataclass_from_dict(
                Orientation,
                {"roll": data["roll"], "pitch": data["pitch"], "yaw": data["yaw"]},
            ),
        )
        for data in kinematic_config_dict.values()
    ]

    # Create a Kinematics object using the components list as arguments
    kinematics = Kinematics(*components)
    return kinematics


if __name__ == "__main__":
    default_kinematics_path = r"/home/bot/moveit2_ws/src/robot-description-setup-assistant/resources/config/ur5/default_kinematics.yaml"
    a: Kinematics = create_kinematics_from_yaml(default_kinematics_path)

    print(a.shoulder.orientation)
