"""Pydantic models mirroring the C++ robot config structs."""
from __future__ import annotations

from pydantic import BaseModel, Field


class CategoryInfo(BaseModel):
    id: str = ""
    display_name: str = ""
    description: str = ""
    manufacturer: str = ""
    website: str = ""


class RobotSpecifications(BaseModel):
    degrees_of_freedom: int = 0
    payload_kg: float = 0.0
    reach_mm: float = 0.0
    weight_kg: float = 0.0
    repeatability_mm: float = 0.0
    max_speed_ms: float = 0.0
    mounting_options: list[str] = Field(default_factory=list)
    safety_certified: bool = False
    collaborative: bool = False
    torque_sensing: bool = False


class AttachInfo(BaseModel):
    """Mount contract. Arms expose `tool_frame`; end-effectors expose
    `mount_frame` + an xyz/rpy offset onto the arm's tool_frame."""

    tool_frame: str = ""
    mount_frame: str = ""
    xyz: list[float] = Field(default_factory=lambda: [0.0, 0.0, 0.0])
    rpy: list[float] = Field(default_factory=lambda: [0.0, 0.0, 0.0])


class RobotConfig(BaseModel):
    id: str
    display_name: str
    description: str = ""
    image_path: str = ""
    urdf_package: str
    urdf_path: str = ""
    xacro_args: str = ""
    category: str = ""
    # Component kind: "arm" (default), "end_effector", or "base".
    type: str = "arm"
    attach: AttachInfo = Field(default_factory=AttachInfo)
    specifications: RobotSpecifications = Field(default_factory=RobotSpecifications)
    required_packages: list[str] = Field(default_factory=list)
    optional_packages: list[str] = Field(default_factory=list)
    tags: list[str] = Field(default_factory=list)


class RobotFilter(BaseModel):
    type: str | None = None
    category: str | None = None
    min_payload: float | None = None
    max_payload: float | None = None
    min_reach: float | None = None
    max_reach: float | None = None
    degrees_of_freedom: int | None = None
    collaborative_only: bool | None = None
    required_tags: list[str] = Field(default_factory=list)
    search_text: str = ""

    def is_empty(self) -> bool:
        return (
            self.type is None
            and self.category is None
            and self.min_payload is None
            and self.max_payload is None
            and self.min_reach is None
            and self.max_reach is None
            and self.degrees_of_freedom is None
            and self.collaborative_only is None
            and not self.required_tags
            and not self.search_text
        )
