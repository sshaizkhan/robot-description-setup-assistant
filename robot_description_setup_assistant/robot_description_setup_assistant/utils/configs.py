import yaml
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


def create_kinematics_class(names):
    components = {name: f"components['{name}']" for name in names}

    class Kinematics:
        def __init__(self, **kwargs):
            for name, value in kwargs.items():
                setattr(self, name, value)

        def __repr__(self):
            args = ", ".join([f"{name}={components[name]}" for name in names])
            return f"Kinematics({args})"

    return Kinematics


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

    Kinematics = create_kinematics_class(kinematic_config_dict.keys())
    # print(kinematic_config_dict)
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


if __name__ == "__main__":
    default_kinematics_path = r"/home/bot/moveit2_ws/src/robot-description-setup-assistant/resources/config/ur5/default_kinematics.yaml"
    a = create_kinematics_from_yaml(default_kinematics_path)

    print(a.random_wrist.position)
    # print(type(a))
