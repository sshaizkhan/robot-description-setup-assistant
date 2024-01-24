from robot_description_setup_assistant.ur_arms.common.ur_macro import URRobot


class UR5Robot(URRobot):
    """
    This class generates the xacro for the UR5
    """

    def __init__(self, prefix: str,
                 joint_limits_parameters_file: str,
                 kinematics_parameters_file: str,
                 physical_parameters_file: str,
                 visual_parameters_file: str,
                 safety_limits: bool = True,
                 safety_pos_margin: float = 0.15,
                 safety_k_position: float = 20,
                 tool_tip_link: str = "tool0",
                 ):
        super().__init__(prefix=prefix,
                         joint_limits_parameters_file=joint_limits_parameters_file,
                         kinematics_parameters_file=kinematics_parameters_file,
                         physical_parameters_file=physical_parameters_file,
                         visual_parameters_file=visual_parameters_file,
                         safety_limits=safety_limits,
                         safety_pos_margin=safety_pos_margin,
                         safety_k_position=safety_k_position,
                         tool_tip_link=tool_tip_link,
                         )
        self._robot_name = "ur5"

    def __generate_macro__(self) -> list:
        return self.to_xml()
