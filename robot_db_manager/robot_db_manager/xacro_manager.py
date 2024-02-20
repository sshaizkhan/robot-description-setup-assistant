import xacro

def process_xacro_to_urdf(xacro_file, xacro_args={}):
    doc = xacro.process_file(xacro_file, mappings=xacro_args)
    robot_desc = doc.toprettyxml(indent='  ')
    return robot_desc

# Example usage
xacro_file = 'rds_ws/src/Universal_Robots_ROS2_Description/urdf/ur.urdf.xacro'
xacro_args = {'name': 'ur5_robot', 'ur_type': 'ur5'}  # Replace with your xacro arguments
urdf_content = process_xacro_to_urdf(xacro_file, xacro_args)
print(urdf_content)
