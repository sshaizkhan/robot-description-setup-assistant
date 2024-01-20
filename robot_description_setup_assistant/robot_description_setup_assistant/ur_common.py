import yaml


class RobotModelData:
    def __init__(self,
                 joint_limits_parameters_file: str,
                 kinematics_parameters_file: str,
                 physical_parameters_file: str,
                 visual_parameters_file: str
                 ):
        # Read .yaml files from disk, load content into properties
        self.config_joint_limit_parameters = self._load_yaml(
            joint_limits_parameters_file)
        self.config_kinematics_parameters = self._load_yaml(
            kinematics_parameters_file)
        self.config_physical_parameters = self._load_yaml(
            physical_parameters_file)
        self.config_visual_parameters = self._load_yaml(
            visual_parameters_file)

        # Initialize all methods
        self.subsections_from_yaml_dict()
        self.joint_limit_parameters()
        self.dh_parameters()
        self.kinematics_parameters()
        self.offsets_parameters()
        self.mass_parameters()
        self.link_inertia_parameters()
        self.center_of_mass_parameters()
        self.cyclinder_radius_parameters()
        self.mesh_files_parameters()

    @staticmethod
    def _load_yaml(file_path):
        with open(file_path) as file:
            return yaml.safe_load(file)

    # Extract subsections from yaml dictionaries
    def subsections_from_yaml_dict(self):
        self.sec_limits = self.config_joint_limit_parameters['joint_limits']
        self.sec_dh_parameters = self.config_physical_parameters['dh_parameters']
        self.sec_offsets = self.config_physical_parameters['offsets']
        self.sec_inertia_parameters = self.config_physical_parameters['inertia_parameters']
        self.sec_mesh_files = self.config_visual_parameters['mesh_files']
        self.sec_kinematics = self.config_kinematics_parameters['kinematics']

    def joint_limit_parameters(self):
        # Shoulder Pan Joints
        self.shoulder_pan_lower_limit = str(
            self.sec_limits['shoulder_pan']['min_position'])
        self.shoulder_pan_upper_limit = str(
            self.sec_limits['shoulder_pan']['max_position'])
        self.shoulder_pan_velocity_limit = str(
            self.sec_limits['shoulder_pan']['max_velocity'])
        self.shoulder_pan_effort_limit = str(
            self.sec_limits['shoulder_pan']['max_effort'])

        # Shoulder Lift Joints
        self.shoulder_lift_lower_limit = str(
            self.sec_limits['shoulder_lift']['min_position'])
        self.shoulder_lift_upper_limit = str(
            self.sec_limits['shoulder_lift']['max_position'])
        self.shoulder_lift_velocity_limit = str(
            self.sec_limits['shoulder_lift']['max_velocity'])
        self.shoulder_lift_effort_limit = str(
            self.sec_limits['shoulder_lift']['max_effort'])

        # Elbow Joints
        self.elbow_joint_lower_limit = str(
            self.sec_limits['elbow_joint']['min_position'])
        self.elbow_joint_upper_limit = str(
            self.sec_limits['elbow_joint']['max_position'])
        self.elbow_joint_velocity_limit = str(
            self.sec_limits['elbow_joint']['max_velocity'])
        self.elbow_joint_effort_limit = str(
            self.sec_limits['elbow_joint']['max_effort'])

        # Wrist 1 Joints
        self.wrist_1_lower_limit = str(
            self.sec_limits['wrist_1']['min_position'])
        self.wrist_1_upper_limit = str(
            self.sec_limits['wrist_1']['max_position'])
        self.wrist_1_velocity_limit = str(
            self.sec_limits['wrist_1']['max_velocity'])
        self.wrist_1_effort_limit = str(
            self.sec_limits['wrist_1']['max_effort'])

        # Wrist 2 Joints
        self.wrist_2_lower_limit = str(
            self.sec_limits['wrist_2']['min_position'])
        self.wrist_2_upper_limit = str(
            self.sec_limits['wrist_2']['max_position'])
        self.wrist_2_velocity_limit = str(
            self.sec_limits['wrist_2']['max_velocity'])
        self.wrist_2_effort_limit = str(
            self.sec_limits['wrist_2']['max_effort'])

        # Wrist 3 Joints
        self.wrist_3_lower_limit = str(
            self.sec_limits['wrist_3']['min_position'])
        self.wrist_3_upper_limit = str(
            self.sec_limits['wrist_3']['max_position'])
        self.wrist_3_velocity_limit = str(
            self.sec_limits['wrist_3']['max_velocity'])
        self.wrist_3_effort_limit = str(
            self.sec_limits['wrist_3']['max_effort'])

    def dh_parameters(self):
        self.d1 = str(self.sec_dh_parameters['d1'])
        self.a2 = str(self.sec_dh_parameters['a2'])
        self.a3 = str(self.sec_dh_parameters['a3'])
        self.d4 = str(self.sec_dh_parameters['d4'])
        self.d5 = str(self.sec_dh_parameters['d5'])
        self.d6 = str(self.sec_dh_parameters['d6'])

    def kinematics_parameters(self):
        # Shoulder kinematics
        self.shoulder_x = str(self.sec_kinematics['shoulder']['position']['x'])
        self.shoulder_y = str(self.sec_kinematics['shoulder']['position']['y'])
        self.shoulder_z = str(self.sec_kinematics['shoulder']['position']['z'])
        self.shoulder_roll = str(
            self.sec_kinematics['shoulder']['orientation']['roll'])
        self.shoulder_pitch = str(
            self.sec_kinematics['shoulder']['orientation']['pitch'])
        self.shoulder_yaw = str(
            self.sec_kinematics['shoulder']['orientation']['yaw'])

        # Upper arm kinematics
        self.upper_arm_x = str(
            self.sec_kinematics['upper_arm']['position']['x'])
        self.upper_arm_y = str(
            self.sec_kinematics['upper_arm']['position']['y'])
        self.upper_arm_z = str(
            self.sec_kinematics['upper_arm']['position']['z'])
        self.upper_arm_roll = str(
            self.sec_kinematics['upper_arm']['orientation']['roll'])
        self.upper_arm_pitch = str(
            self.sec_kinematics['upper_arm']['orientation']['pitch'])
        self.upper_arm_yaw = str(
            self.sec_kinematics['upper_arm']['orientation']['yaw'])

        # Forearm kinematics
        self.forearm_x = str(self.sec_kinematics['forearm']['position']['x'])
        self.forearm_y = str(self.sec_kinematics['forearm']['position']['y'])
        self.forearm_z = str(self.sec_kinematics['forearm']['position']['z'])
        self.forearm_roll = str(
            self.sec_kinematics['forearm']['orientation']['roll'])
        self.forearm_pitch = str(
            self.sec_kinematics['forearm']['orientation']['pitch'])
        self.forearm_yaw = str(
            self.sec_kinematics['forearm']['orientation']['yaw'])

        # Wrist 1 kinematics
        self.wrist_1_x = str(self.sec_kinematics['wrist_1']['position']['x'])
        self.wrist_1_y = str(self.sec_kinematics['wrist_1']['position']['y'])
        self.wrist_1_z = str(self.sec_kinematics['wrist_1']['position']['z'])
        self.wrist_1_roll = str(
            self.sec_kinematics['wrist_1']['orientation']['roll'])
        self.wrist_1_pitch = str(
            self.sec_kinematics['wrist_1']['orientation']['pitch'])
        self.wrist_1_yaw = str(
            self.sec_kinematics['wrist_1']['orientation']['yaw'])

        # Wrist 2 kinematics
        self.wrist_2_x = str(self.sec_kinematics['wrist_2']['position']['x'])
        self.wrist_2_y = str(self.sec_kinematics['wrist_2']['position']['y'])
        self.wrist_2_z = str(self.sec_kinematics['wrist_2']['position']['z'])
        self.wrist_2_roll = str(
            self.sec_kinematics['wrist_2']['orientation']['roll'])
        self.wrist_2_pitch = str(
            self.sec_kinematics['wrist_2']['orientation']['pitch'])
        self.wrist_2_yaw = str(
            self.sec_kinematics['wrist_2']['orientation']['yaw'])

        # Wrist 3 kinematics
        self.wrist_3_x = str(self.sec_kinematics['wrist_3']['position']['x'])
        self.wrist_3_y = str(self.sec_kinematics['wrist_3']['position']['y'])
        self.wrist_3_z = str(self.sec_kinematics['wrist_3']['position']['z'])
        self.wrist_3_roll = str(
            self.sec_kinematics['wrist_3']['orientation']['roll'])
        self.wrist_3_pitch = str(
            self.sec_kinematics['wrist_3']['orientation']['pitch'])
        self.wrist_3_yaw = str(
            self.sec_kinematics['wrist_3']['orientation']['yaw'])

    def offsets_parameters(self):
        self.shoulder_offset = str(self.sec_offsets['shoulder_offset'])
        self.elbow_offset = str(self.sec_offsets['elbow_offset'])

    def mass_parameters(self):
        self.base_mass = str(self.sec_inertia_parameters['base_mass'])
        self.shoulder_mass = str(self.sec_inertia_parameters['shoulder_mass'])
        self.upper_arm_mass = str(
            self.sec_inertia_parameters['upper_arm_mass'])
        self.upper_arm_inertia_offset = str(
            self.sec_inertia_parameters['upper_arm_inertia_offset'])
        self.forearm_mass = str(self.sec_inertia_parameters['forearm_mass'])
        self.wrist_1_mass = str(self.sec_inertia_parameters['wrist_1_mass'])
        self.wrist_2_mass = str(self.sec_inertia_parameters['wrist_2_mass'])
        self.wrist_3_mass = str(self.sec_inertia_parameters['wrist_3_mass'])

    def link_inertia_parameters(self):
        self.intertia_links = self.sec_inertia_parameters['links']
        self.base_inertia_radius = str(self.intertia_links['base']['radius'])
        self.base_inertia_length = str(self.intertia_links['base']['length'])
        self.shoulder_inertia_radius = str(
            self.intertia_links['shoulder']['radius'])
        self.shoulder_inertia_length = str(
            self.intertia_links['shoulder']['length'])
        self.upperarm_inertia_radius = str(
            self.intertia_links['upperarm']['radius'])
        self.upperarm_inertia_length = str(
            self.intertia_links['upperarm']['length'])
        self.forearm_inertia_radius = str(
            self.intertia_links['forearm']['radius'])
        self.forearm_inertia_length = str(
            self.intertia_links['forearm']['length'])
        self.wrist_1_inertia_radius = str(
            self.intertia_links['wrist_1']['radius'])
        self.wrist_1_inertia_length = str(
            self.intertia_links['wrist_1']['length'])
        self.wrist_2_inertia_radius = str(
            self.intertia_links['wrist_2']['radius'])
        self.wrist_2_inertia_length = str(
            self.intertia_links['wrist_2']['length'])
        self.wrist_3_inertia_radius = str(
            self.intertia_links['wrist_3']['radius'])
        self.wrist_3_inertia_length = str(
            self.intertia_links['wrist_3']['length'])

    def center_of_mass_parameters(self):
        self.prop_shoulder_cog = self.sec_inertia_parameters['center_of_mass']['shoulder_cog']
        self.prop_upper_arm_cog = self.sec_inertia_parameters['center_of_mass']['upper_arm_cog']
        self.prop_forearm_cog = self.sec_inertia_parameters['center_of_mass']['forearm_cog']
        self.prop_wrist_1_cog = self.sec_inertia_parameters['center_of_mass']['wrist_1_cog']
        self.prop_wrist_2_cog = self.sec_inertia_parameters['center_of_mass']['wrist_2_cog']
        self.prop_wrist_3_cog = self.sec_inertia_parameters['center_of_mass']['wrist_3_cog']

        self.shoulder_cog = " ".join([str(self.prop_shoulder_cog['x']), str(
            self.prop_shoulder_cog['y']), str(self.prop_shoulder_cog['z'])])

        self.upper_cog = " ".join([str(self.prop_upper_arm_cog['x']), str(
            self.prop_upper_arm_cog['y']), str(self.prop_upper_arm_cog['z'])])

        self.forearm_cog = " ".join([str(self.prop_forearm_cog['x']), str(
            self.prop_forearm_cog['y']), str(self.prop_forearm_cog['z'])])

        self.wrist_1_cog = " ".join([str(self.prop_wrist_1_cog['x']), str(
            self.prop_wrist_1_cog['y']), str(self.prop_wrist_1_cog['z'])])

        self.wrist_2_cog = " ".join([str(self.prop_wrist_2_cog['x']), str(
            self.prop_wrist_2_cog['y']), str(self.prop_wrist_2_cog['z'])])

        self.wrist_3_cog = " ".join([str(self.prop_wrist_3_cog['x']), str(
            self.prop_wrist_3_cog['y']), str(self.prop_wrist_3_cog['z'])])

    def cyclinder_radius_parameters(self):
        self.shoulder_radius = str(
            self.sec_inertia_parameters['shoulder_radius'])
        self.upper_arm_radius = str(
            self.sec_inertia_parameters['upper_arm_radius'])
        self.elbow_radius = str(self.sec_inertia_parameters['elbow_radius'])
        self.forearm_radius = str(
            self.sec_inertia_parameters['forearm_radius'])
        self.wrist_radius = str(self.sec_inertia_parameters['wrist_radius'])

    def mesh_files_parameters(self):
        # Base
        base_mesh = self.sec_mesh_files['base']
        self.base_visual_mesh = str(base_mesh['visual']['mesh'])
        self.base_visual_material_name = str(
            base_mesh['visual']['material']['name'])
        self.base_visual_material_color = str(
            base_mesh['visual']['material']['color'])
        self.base_collision_mesh = str(base_mesh['collision']['mesh'])

        # Shoulder
        shoulder_mesh = self.sec_mesh_files['shoulder']
        self.shoulder_visual_mesh = str(shoulder_mesh['visual']['mesh'])
        self.shoulder_visual_material_name = str(
            shoulder_mesh['visual']['material']['name'])
        self.shoulder_visual_material_color = str(
            shoulder_mesh['visual']['material']['color'])
        self.shoulder_collision_mesh = str(shoulder_mesh['collision']['mesh'])

        # Upper Arm
        upper_arm_mesh = self.sec_mesh_files['upper_arm']
        self.upper_arm_visual_mesh = str(upper_arm_mesh['visual']['mesh'])
        self.upper_arm_visual_material_name = str(
            upper_arm_mesh['visual']['material']['name'])
        self.upper_arm_visual_material_color = str(
            upper_arm_mesh['visual']['material']['color'])
        self.upper_arm_collision_mesh = str(
            upper_arm_mesh['collision']['mesh'])

        # Forearm
        forearm_mesh = self.sec_mesh_files['forearm']
        self.forearm_visual_mesh = str(forearm_mesh['visual']['mesh'])
        self.forearm_visual_material_name = str(
            forearm_mesh['visual']['material']['name'])
        self.forearm_visual_material_color = str(
            forearm_mesh['visual']['material']['color'])
        self.forearm_collision_mesh = str(forearm_mesh['collision']['mesh'])

        # Wrist 1
        wrist_1_mesh = self.sec_mesh_files['wrist_1']
        self.wrist_1_visual_mesh = str(wrist_1_mesh['visual']['mesh'])
        self.wrist_1_visual_material_name = str(
            wrist_1_mesh['visual']['material']['name'])
        self.wrist_1_visual_material_color = str(
            wrist_1_mesh['visual']['material']['color'])
        self.wrist_1_collision_mesh = str(wrist_1_mesh['collision']['mesh'])
        self.wrist_1_visual_offset = str(wrist_1_mesh['visual_offset'])

        # Wrist 2
        wrist_2_mesh = self.sec_mesh_files['wrist_2']
        self.wrist_2_visual_mesh = str(wrist_2_mesh['visual']['mesh'])
        self.wrist_2_visual_material_name = str(
            wrist_2_mesh['visual']['material']['name'])
        self.wrist_2_visual_material_color = str(
            wrist_2_mesh['visual']['material']['color'])
        self.wrist_2_collision_mesh = str(wrist_2_mesh['collision']['mesh'])
        self.wrist_2_visual_offset = str(wrist_2_mesh['visual_offset'])

        # Wrist 3
        wrist_3_mesh = self.sec_mesh_files['wrist_3']
        self.wrist_3_visual_mesh = str(wrist_3_mesh['visual']['mesh'])
        self.wrist_3_visual_material_name = str(
            wrist_3_mesh['visual']['material']['name'])
        self.wrist_3_visual_material_color = str(
            wrist_3_mesh['visual']['material']['color'])
        self.wrist_3_collision_mesh = str(wrist_3_mesh['collision']['mesh'])
        self.wrist_3_visual_offset = str(wrist_3_mesh['visual_offset'])


if __name__ == "__main__":
    default_kinematics_path = '/home/bot/rds_ws/src/robot_description_app/robot-description-setup-assistant/robot_description_resources/config/ur5/default_kinematics.yaml'
    joint_limits_config_path = '/home/bot/rds_ws/src/robot_description_app/robot-description-setup-assistant/robot_description_resources/config/ur5/joint_limits.yaml'
    physical_param_config_path = '/home/bot/rds_ws/src/robot_description_app/robot-description-setup-assistant/robot_description_resources/config/ur5/physical_parameters.yaml'
    visual_param_config_path = '/home/bot/rds_ws/src/robot_description_app/robot-description-setup-assistant/robot_description_resources/config/ur5/visual_parameters.yaml'

    model_data = RobotModelData(joint_limits_config_path, default_kinematics_path,
                                physical_param_config_path, visual_param_config_path)

    # Accessing specific parameters

    print(model_data.wrist_3_visual_mesh)
    print(type(model_data.wrist_3_visual_mesh))
