from lxml import etree


class XacroGenerator:
    def __init__(self, robot_name, description_pkg, arm_id='panda'):
        self.NS_XACRO = "http://www.ros.org/wiki/xacro"
        self.description_pkg = description_pkg
        self.arm_id = arm_id
        self.root = etree.Element("robot", name=robot_name, nsmap={
                                  "xacro": self.NS_XACRO})
        self.macro = self.create_macro()

    def create_macro(self):
        return etree.SubElement(self.root, "{%s}macro" % self.NS_XACRO, name="panda_arm", params=f"arm_id:='{self.arm_id}' description_pkg:='{self.description_pkg}' connected_to:='' xyz:='0 0 0' rpy:='0 0 0'")

    def create_unless(self):
        unless = etree.SubElement(
            self.macro, "{%s}unless" % self.NS_XACRO, value="${not connected_to}")
        joint = etree.SubElement(
            unless, "joint", name="${%s}_joint_${connected_to}" % self.arm_id, type="fixed")
        etree.SubElement(joint, "parent", link="${connected_to}")
        etree.SubElement(joint, "child", link="${%s}_link0" % self.arm_id)
        etree.SubElement(joint, "origin", rpy="${rpy}", xyz="${xyz}")

    def create_link_and_joint(self, link_num, joint_type, joint_params, origin_params, mesh_visual, mesh_collision):
        link = etree.SubElement(
            self.macro, "link", name="${%s}_link%d" % (self.arm_id, link_num))
        visual = etree.SubElement(link, "visual")
        geometry_v = etree.SubElement(visual, "geometry")
        etree.SubElement(
            geometry_v, "mesh", filename="package://${%s}/meshes/visual/%s" % (self.description_pkg, mesh_visual))

        collision = etree.SubElement(link, "collision")
        geometry_c = etree.SubElement(collision, "geometry")
        etree.SubElement(geometry_c, "mesh", filename="package://${%s}/meshes/collision/%s" % (
            self.description_pkg, mesh_collision))

        joint = etree.SubElement(self.macro, "joint", name="${%s}_joint%d" % (
            self.arm_id, link_num), type=joint_type)
        for k, v in joint_params.items():
            etree.SubElement(joint, k, **v)
        etree.SubElement(joint, "origin", **origin_params)
        etree.SubElement(joint, "parent", link="${%s}_link%d" % (
            self.arm_id, link_num - 1))
        etree.SubElement(
            joint, "child", link="${%s}_link%d" % (self.arm_id, link_num))
        etree.SubElement(joint, "axis", xyz="0 0 1")

    def generate_links_and_joints(self):
        for i in range(1, 8):
            joint_type = "revolute"
            joint_params = {
                "safety_controller": {"k_position": "100.0", "k_velocity": "40.0", "soft_lower_limit": "-2.8973", "soft_upper_limit": "2.8973"},
                "limit": {"effort": "87", "lower": "-2.8973", "upper": "2.8973", "velocity": "2.1750"}
            }
            origin_params = {"rpy": "0 0 0", "xyz": "0 0 0"}
            if i in [2, 3, 5, 6]:
                joint_params["origin"] = {
                    "rpy": "${-pi/2} 0 0", "xyz": "0 0 0"}
            if i in [4, 7]:
                joint_params["origin"] = {"rpy": "${pi/2} 0 0", "xyz": "0 0 0"}
            self.create_link_and_joint(
                i, joint_type, joint_params, origin_params, f"link{i}.dae", f"link{i}.stl")

    def add_final_link_and_joint(self):
        final_link = etree.SubElement(
            self.macro, "link", name="${%s}_link8" % self.arm_id)
        final_joint = etree.SubElement(
            self.macro, "joint", name="${%s}_joint8" % self.arm_id, type="fixed")
        etree.SubElement(final_joint, "origin", rpy="0 0 0", xyz="0 0 0.107")
        etree.SubElement(final_joint, "parent",
                         link="${%s}_link7" % self.arm_id)
        etree.SubElement(final_joint, "child",
                         link="${%s}_link8" % self.arm_id)

    def write_to_file(self, file_path):
        with open(file_path, "wb") as f:
            f.write(etree.tostring(self.root, xml_declaration=True,
                    encoding="UTF-8", pretty_print=True))


if __name__ == "__main__":
    generator = XacroGenerator(
        robot_name="panda", description_pkg="franka_description")
    generator.create_unless()
    generator.generate_links_and_joints()
    generator.add_final_link_and_joint()
    generator.write_to_file(
        "/home/bot/rds_ws/src/robot_description_app/robot-description-setup-assistant/robot_description_setup_assistant/generated_xacro/panda_robot.xacro")
