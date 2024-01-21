from enum import Enum
from dataclasses import dataclass


class JointType(Enum):
    PRISMATIC = "prismatic"
    REVOLUTE = "revolute"
    FIXED = "fixed"
    FLOATING = "floating"
    PLANAR = "planar"
    CONTINUOUS = "continuous"


@dataclass
class JointAttributes:
    joint_name: str
    type: JointType = JointType.REVOLUTE
