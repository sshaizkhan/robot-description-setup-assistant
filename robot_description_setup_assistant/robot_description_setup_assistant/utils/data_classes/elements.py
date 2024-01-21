from typing import Tuple, Optional
from dataclasses import dataclass


@dataclass
class Origin:
    xyz: Tuple[float, float, float] = (0, 0, 0)
    rpy: Tuple[float, float, float] = (0, 0, 0)


@dataclass
class Calibration:
    rising: Optional[float] = 0.0
    falling: Optional[float] = 0.0


@dataclass
class Dynamics:
    damping: Optional[float] = 0.0
    friction: Optional[float] = 0.0


@dataclass
class Limits:
    lower: Optional[float] = 0.0
    upper: Optional[float] = 0.0
    effort: Optional[float] = 0.0
    velocity: Optional[float] = 0.0


@dataclass
class JointMimic:
    joint: str
    multiplier: Optional[float] = None
    offset: Optional[float] = None


@dataclass
class SafetyController:
    soft_lower_limit: float
    soft_upper_limit: float
    k_position: float
    k_velocity: float


@dataclass
class SafetyParams:
    safety_pos_margin: float = 0.0
    safety_k_position: float = 0.0
