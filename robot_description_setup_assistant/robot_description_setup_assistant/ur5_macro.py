import math
from robot_description_setup_assistant.ur_macro import URRobot, Origin, JointLimits


class UR5Robot(URRobot):

    def __init__(self, robot_name,
                 prefix,
                 joint_limits_parameters_file,
                 kinematics_parameters_file,
                 physical_parameters_file,
                 visual_parameters_file,
                 safety_limits=True,
                 safety_pos_margin=0.15,
                 safety_k_position=20.0
                 ):
        super().__init__(
            robot_name,
            prefix,
            joint_limits_parameters_file,
            kinematics_parameters_file,
            physical_parameters_file,
            visual_parameters_file,
            safety_limits,
            safety_pos_margin,
            safety_k_position
        )


if __name__ == "__main__":

    joint_limits_config_path = '/home/bot/rds_ws/src/robot_description_app/robot-description-setup-assistant/robot_description_resources/config/ur5/joint_limits.yaml'
    default_kinematics_path = '/home/bot/rds_ws/src/robot_description_app/robot-description-setup-assistant/robot_description_resources/config/ur5/default_kinematics.yaml'
    physical_param_config_path = '/home/bot/rds_ws/src/robot_description_app/robot-description-setup-assistant/robot_description_resources/config/ur5/physical_parameters.yaml'
    visual_param_config_path = '/home/bot/rds_ws/src/robot_description_app/robot-description-setup-assistant/robot_description_resources/config/ur5/visual_parameters.yaml'

    ur5 = UR5Robot(
        "ur5_robot",
        "",
        joint_limits_config_path,
        default_kinematics_path,
        physical_param_config_path,
        visual_param_config_path
    )

    # Base Link
    ur5.create_link(
        "base_link",
        Origin(xyz=("0", "0", "0"), rpy=(
            "0", "0", str(math.pi))),
        ur5.base_visual_mesh,
        ur5.base_visual_material_name,
        ur5.base_visual_material_color,
        Origin(xyz=("0", "0", "0"),
               rpy=("0", "0", str(math.pi))),
        ur5.base_collision_mesh,
        Origin(xyz=("0", "0", "0"), rpy=("0", "0", "0")),
        ur5.base_inertia_radius,
        ur5.base_inertia_length,
        ur5.base_mass
    )

    # Shoulder Link
    ur5.create_link(
        "shoulder_link",
        Origin(xyz=("0", "0", "0"), rpy=(
            "0", "0", str(math.pi))),
        ur5.shoulder_visual_mesh,
        ur5.shoulder_visual_material_name,
        ur5.shoulder_visual_material_color,
        Origin(xyz=("0", "0", "0"),
               rpy=("0", "0", str(math.pi))),
        ur5.shoulder_collision_mesh,
        Origin(xyz=("0", "0", "0"), rpy=("0", "0", "0")),
        ur5.shoulder_inertia_radius,
        ur5.shoulder_inertia_length,
        ur5.shoulder_mass
    )

    # Shoulder Pan Joint
    ur5.create_joint(
        "shoulder_pan_joint",
        "base_link",
        "shoulder_link",
        Origin(xyz=(ur5.shoulder_x, ur5.shoulder_y, ur5.shoulder_z),
               rpy=(ur5.shoulder_roll, ur5.shoulder_pitch, ur5.shoulder_yaw)),
        ("0", "0", "1"),
        JointLimits(
            ur5.shoulder_pan_lower_limit,
            ur5.shoulder_pan_upper_limit,
            ur5.shoulder_pan_effort_limit,
            ur5.shoulder_pan_velocity_limit
        ),
        joint_type='revolute'
    )

    # Upper Arm Link
    ur5.create_link(
        "upper_arm_link",
        Origin(xyz=("0", "0", ur5.shoulder_offset),
               rpy=(str(math.pi/2), "0", str(-1 * math.pi/2))),
        ur5.upper_arm_visual_mesh,
        ur5.upper_arm_visual_material_name,
        ur5.upper_arm_visual_material_color,
        Origin(xyz=("0", "0", ur5.shoulder_offset),
               rpy=(str(math.pi/2), "0", str(-1 * math.pi/2))),
        ur5.upper_arm_collision_mesh,
        Origin(xyz=(str(-0.5 * float(ur5.upperarm_inertia_length)), "0.0", ur5.upper_arm_inertia_offset),
               rpy=("0", str(math.pi/2), "0")),
        ur5.upperarm_inertia_radius,
        ur5.upperarm_inertia_length,
        ur5.upper_arm_mass
    )

    # Shoulder Lift Joint
    ur5.create_joint(
        "shoulder_lift_joint",
        "shoulder_link",
        "upper_arm_link",
        Origin(xyz=(ur5.upper_arm_x, ur5.upper_arm_y, ur5.upper_arm_z),
               rpy=(ur5.upper_arm_roll, ur5.upper_arm_pitch, ur5.upper_arm_yaw)),
        ("0", "0", "1"),
        JointLimits(
            ur5.shoulder_lift_lower_limit,
            ur5.shoulder_lift_upper_limit,
            ur5.shoulder_lift_effort_limit,
            ur5.shoulder_lift_velocity_limit
        ),
        joint_type='revolute'
    )

    # Forearm Link
    ur5.create_link(
        "forearm_link",
        Origin(xyz=("0", "0", ur5.elbow_offset),
               rpy=(str(math.pi/2), "0", str(-1*(math.pi)/2))),
        ur5.forearm_visual_mesh,
        ur5.forearm_visual_material_name,
        ur5.forearm_visual_material_color,
        Origin(xyz=("0", "0", ur5.elbow_offset),
               rpy=(str(math.pi/2), "0", str(-1*(math.pi)/2))),
        ur5.forearm_collision_mesh,
        Origin(xyz=(str(-0.5*float(ur5.forearm_inertia_length)), "0.0.", ur5.elbow_offset),
               rpy=("0", str(math.pi/2), "0")),
        ur5.forearm_inertia_radius,
        ur5.forearm_inertia_length,
        ur5.forearm_mass
    )

    # Elbow Joint
    ur5.create_joint(
        "elbow_joint",
        "upper_arm_link",
        "forearm_link",
        Origin(xyz=(ur5.forearm_x, ur5.forearm_y, ur5.forearm_z),
               rpy=(ur5.forearm_roll, ur5.forearm_pitch, ur5.forearm_yaw)),
        ("0", "0", "1"),
        JointLimits(
            ur5.elbow_joint_lower_limit,
            ur5.elbow_joint_upper_limit,
            ur5.elbow_joint_effort_limit,
            ur5.elbow_joint_velocity_limit
        ),
        joint_type='revolute'
    )

    # Wrist 1 Link
    ur5.create_link(
        "wrist_1_link",
        Origin(xyz=("0", "0", ur5.wrist_1_visual_offset),
               rpy=(str(math.pi/2), "0", "0")),
        ur5.wrist_1_visual_mesh,
        ur5.wrist_1_visual_material_name,
        ur5.wrist_1_visual_material_color,
        Origin(xyz=("0", "0", ur5.wrist_1_visual_offset),
               rpy=(str(math.pi/2), "0", "0")),
        ur5.wrist_1_collision_mesh,
        Origin(xyz=("0", "0", "0"), rpy=("0", "0", "0")),
        ur5.wrist_1_inertia_radius,
        ur5.wrist_1_inertia_length,
        ur5.wrist_1_mass
    )

    # Wrist 1 Joint
    ur5.create_joint(
        "wirst_1_joint",
        "forearm_link",
        "wrist_1_link",
        Origin(xyz=(ur5.wrist_1_x, ur5.wrist_1_y, ur5.wrist_1_z),
               rpy=(ur5.wrist_1_roll, ur5.wrist_1_pitch, ur5.wrist_1_yaw)),
        ("0", "0", "1"),
        JointLimits(
            ur5.wrist_1_lower_limit,
            ur5.wrist_1_upper_limit,
            ur5.wrist_1_effort_limit,
            ur5.wrist_1_velocity_limit
        ),
        joint_type='revolute'
    )

    # Wrist 2 Link
    ur5.create_link(
        "wrist_2_link",
        Origin(xyz=("0", "0", ur5.wrist_2_visual_offset),
               rpy=("0", "0", "0")),
        ur5.wrist_2_visual_mesh,
        ur5.wrist_2_visual_material_name,
        ur5.wrist_2_visual_material_color,
        Origin(xyz=("0", "0", ur5.wrist_2_visual_offset),
               rpy=("0", "0", "0")),
        ur5.wrist_2_collision_mesh,
        Origin(xyz=("0", "0", "0"), rpy=("0", "0", "0")),
        ur5.wrist_2_inertia_radius,
        ur5.wrist_2_inertia_length,
        ur5.wrist_2_mass
    )

    # Wrist 2 Joint
    ur5.create_joint(
        "wirst_2_joint",
        "wrist_1_link",
        "wrist_2_link",
        Origin(xyz=(ur5.wrist_2_x, ur5.wrist_2_y, ur5.wrist_2_z),
               rpy=(ur5.wrist_2_roll, ur5.wrist_2_pitch, ur5.wrist_2_yaw)),
        ("0", "0", "1"),
        JointLimits(
            ur5.wrist_2_lower_limit,
            ur5.wrist_2_upper_limit,
            ur5.wrist_2_effort_limit,
            ur5.wrist_2_velocity_limit
        ),
        joint_type='revolute'
    )

    # Wrist 3 Link
    ur5.create_link(
        "wrist_3_link",
        Origin(xyz=("0", "0", ur5.wrist_3_visual_offset),
               rpy=(str(math.pi/2), "0", "0")),
        ur5.wrist_3_visual_mesh,
        ur5.wrist_3_visual_material_name,
        ur5.wrist_3_visual_material_color,
        Origin(xyz=("0", "0", ur5.wrist_3_visual_offset),
               rpy=(str(math.pi/2), "0", "0")),
        ur5.wrist_3_collision_mesh,
        Origin(xyz=("0", "0", str(-0.5*float(ur5.wrist_3_inertia_length))),
               rpy=("0", "0", "0")),
        ur5.wrist_3_inertia_radius,
        ur5.wrist_3_inertia_length,
        ur5.wrist_3_mass
    )

    # Wrist 3 Joint
    ur5.create_joint(
        "wrist_3_joint",
        "wrist_2_link",
        "wrist_3_link",
        Origin(xyz=(ur5.wrist_3_x, ur5.wrist_3_y, ur5.wrist_3_z),
               rpy=(ur5.wrist_3_roll, ur5.wrist_3_pitch, ur5.wrist_3_yaw)),
        ("0", "0", "1"),
        JointLimits(
            ur5.wrist_3_lower_limit,
            ur5.wrist_3_upper_limit,
            ur5.wrist_3_effort_limit,
            ur5.wrist_3_velocity_limit
        ),
        joint_type='revolute'
    )

    # Tool0 Link
    ur5.create_joint(
        joint_name="tool0_joint",
        parent_link="wrist_3_link",
        child_link="tool0",
        joint_origin=Origin(xyz=(ur5.wrist_3_x, "0", ur5.wrist_3_z),
                            rpy=("0", "0", "0")),
        joint_type="fixed"
    )

    # Tool0 Link
    ur5.create_link(
        "tool0",
        collision_origin=Origin(xyz=("0", "0", "0"),
                                rpy=("0", "0", "0")),
        geometry_type="box",
        geometry_dimensions=["0.1", "0.1", "0.1"]
    )
#
    ur5.write_to_file(
        "/home/bot/rds_ws/src/robot_description_app/robot-description-setup-assistant/robot_description_setup_assistant/generated_xacro/test_ur5_robot.xacro"
    )
