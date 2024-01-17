from lxml import etree

# Define the Xacro namespace
NS_XACRO = "http://www.ros.org/wiki/xacro"

# Create the root element with the specified namespace
root = etree.Element("robot", name="panda", nsmap={"xacro": NS_XACRO})

# Create the xacro:macro element
macro = etree.SubElement(root, "{%s}macro" % NS_XACRO, name="panda_arm", params="arm_id:='panda' description_pkg:='franka_description' connected_to:='' xyz:='0 0 0' rpy:='0 0 0'")

# Create the xacro:unless element for the joint
unless = etree.SubElement(macro, "{%s}unless" % NS_XACRO, value="${not connected_to}")

# The first joint element
joint = etree.SubElement(unless, "joint", name="${arm_id}_joint_${connected_to}", type="fixed")
etree.SubElement(joint, "parent", link="${connected_to}")
etree.SubElement(joint, "child", link="${arm_id}_link0")
etree.SubElement(joint, "origin", rpy="${rpy}", xyz="${xyz}")

# Function to create link and joint elements
def create_link_and_joint(arm_id, description_pkg, link_num, joint_type, joint_params, origin_params, mesh_visual, mesh_collision):
    link = etree.SubElement(macro, "link", name="${%s}_link%d" % (arm_id, link_num))
    visual = etree.SubElement(link, "visual")
    geometry_v = etree.SubElement(visual, "geometry")
    etree.SubElement(geometry_v, "mesh", filename="package://${%s}/meshes/visual/%s" % (description_pkg, mesh_visual))

    collision = etree.SubElement(link, "collision")
    geometry_c = etree.SubElement(collision, "geometry")
    etree.SubElement(geometry_c, "mesh", filename="package://${%s}/meshes/collision/%s" % (description_pkg, mesh_collision))

    joint = etree.SubElement(macro, "joint", name="${%s}_joint%d" % (arm_id, link_num), type=joint_type)
    for k, v in joint_params.items():
        etree.SubElement(joint, k, **v)
    etree.SubElement(joint, "origin", **origin_params)
    etree.SubElement(joint, "parent", link="${%s}_link%d"% (arm_id, link_num - 1))
    etree.SubElement(joint, "child", link="${%s}_link%d" % (arm_id, link_num))
    etree.SubElement(joint, "axis", xyz="0 0 1")

# Generate links and joints
for i in range(1, 8):
    joint_type = "revolute"
    joint_params = {
        "safety_controller": {"k_position": "100.0", "k_velocity": "40.0", "soft_lower_limit": "-2.8973", "soft_upper_limit": "2.8973"},
        "limit": {"effort": "87", "lower": "-2.8973", "upper": "2.8973", "velocity": "2.1750"}
    }
    origin_params = {"rpy": "0 0 0", "xyz": "0 0 0"}
    if i in [2, 3, 5, 6]:
        joint_params["origin"] = {"rpy": "${-pi/2} 0 0", "xyz": "0 0 0"}
    if i in [4, 7]:
        joint_params["origin"] = {"rpy": "${pi/2} 0 0", "xyz": "0 0 0"}
    create_link_and_joint("arm_id", "description_pkg", i, joint_type, joint_params, origin_params, f"link{i}.dae", f"link{i}.stl")

# Add final link and joint
final_link = etree.SubElement(macro, "link", name="${arm_id}_link8")
final_joint = etree.SubElement(macro, "joint", name="${arm_id}_joint8", type="fixed")
etree.SubElement(final_joint, "origin", rpy="0 0 0", xyz="0 0 0.107")
etree.SubElement(final_joint, "parent", link="${arm_id}_link7")
etree.SubElement(final_joint, "child", link="${arm_id}_link8")

# Write the Xacro file to disk
with open("/home/bot/rds_ws/src/robot_description_app/robot-description-setup-assistant/robot_description_setup_assistant/generated_xacro/panda_robot.xacro", "wb") as f:
    f.write(etree.tostring(root, xml_declaration=True, encoding="UTF-8", pretty_print=True))
