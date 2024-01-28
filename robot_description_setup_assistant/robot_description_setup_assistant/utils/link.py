import unittest
from lxml import etree
from typing import List, Optional
from robot_description_setup_assistant.utils.data_classes.elements import Origin
from robot_description_setup_assistant.utils.cylinder_inertia import CylinderInertia


class Link:
    def __init__(self, name: str,
                 visual_origin: Optional[Origin] = None,
                 visual_mesh: Optional[str] = None,
                 material_name: Optional[str] = None,
                 material_color: Optional[str] = None,
                 collision_origin: Optional[Origin] = None,
                 collision_mesh: Optional[str] = None,
                 inertial_origin: Optional[Origin] = None,
                 inertial_radius: Optional[str] = None,
                 inertial_length: Optional[str] = None,
                 inertial_mass: Optional[str] = None,
                 geometry_type: str = "mesh",
                 geometry_dimensions: List[str] = None,
                 ):

        self.link_name = name

        self.visual_origin = visual_origin
        self.visual_mesh = visual_mesh
        self.material_name = material_name
        self.material_color = material_color

        self.collision_origin = collision_origin
        self.collision_mesh = collision_mesh

        self.inertial_origin = inertial_origin
        self.inertial_radius = inertial_radius
        self.inertial_length = inertial_length
        self.inertial_mass = inertial_mass

        self.geometry_type = geometry_type
        self.geometry_dimensions = geometry_dimensions

    def to_xml(self):
        link: list = etree.Element("link", name=self.link_name)
        comment = etree.Comment(f"Link {self.link_name.upper()}")
        link.append(comment)

        # Visual element
        if self.visual_origin:
            visual = etree.SubElement(link, "visual")
            etree.SubElement(visual, "origin",
                             xyz=" ".join(map(str, self.visual_origin.xyz)),
                             rpy=" ".join(map(str, self.visual_origin.rpy))
                             )
            geometry_visual = etree.SubElement(visual, "geometry")
            if self.geometry_type == "mesh":
                etree.SubElement(geometry_visual, "mesh",
                                 filename=self.visual_mesh)
            elif self.geometry_type == "cylinder":
                etree.SubElement(geometry_visual, "cylinder",
                                 radius=self.geometry_dimensions[0],
                                 length=self.geometry_dimensions[1])
            elif self.geometry_type == "box":
                etree.SubElement(geometry_visual, "box",
                                 size=" ".join(self.geometry_dimensions))
        # Material element
        if self.material_color and self.material_name:
            material = etree.SubElement(
                visual, "material", name=self.material_name)
            etree.SubElement(material, "color",
                             rgba=self.material_color)

        # Collision element
        if self.collision_origin:
            collision = etree.SubElement(link, "collision")
            etree.SubElement(collision, "origin",
                             xyz=" ".join(map(str, self.collision_origin.xyz)),
                             rpy=" ".join(map(str, self.collision_origin.rpy))
                             )
            geometry_collision = etree.SubElement(collision, "geometry")
            if self.geometry_type == "mesh":
                etree.SubElement(geometry_collision, "mesh",
                                 filename=self.collision_mesh)
            elif self.geometry_type == "cylinder":
                etree.SubElement(geometry_collision, "cylinder",
                                 radius=self.geometry_dimensions[0],
                                 length=self.geometry_dimensions[1])
            elif self.geometry_type == "box":
                etree.SubElement(geometry_collision, "box",
                                 size=" ".join(map(str, self.geometry_dimensions)))

        # Inertial element
        if self.inertial_origin:
            cylindrical_inertia = CylinderInertia(
                self.inertial_radius, self.inertial_length, self.inertial_mass, self.inertial_origin)
            link.append(cylindrical_inertia.to_xml())

        return link


class TestLink(unittest.TestCase):

    def assertXmlEquivalent(self, expected_xml_str, actual_xml_str, print_output=False):
        expected_xml = etree.fromstring(expected_xml_str)
        actual_xml = etree.fromstring(actual_xml_str)

        if print_output:
            print("Expected XML:")
            print(etree.tostring(expected_xml, pretty_print=True).decode())
            print("\nActual XML:")
            print(etree.tostring(actual_xml, pretty_print=True).decode())

        assert etree.tostring(expected_xml) == etree.tostring(actual_xml)

    def test_cylinder_link(self):
        self.maxDiff = None
        link = Link(name="link1",
                    visual_origin=Origin(xyz=(0, 0, 0), rpy=(0, 0, 0)),
                    collision_origin=Origin(xyz=(0, 0, 0), rpy=(0, 0, 0)),
                    geometry_type="cylinder",
                    geometry_dimensions=["0.5", "1.0"]
                    )

        expected_xml = """
<link name="link1">
  <visual>
    <origin xyz="0 0 0" rpy="0 0 0"/>
    <geometry>
      <cylinder radius="0.5" length="1.0"/>
    </geometry>
  </visual>
  <collision>
    <origin xyz="0 0 0" rpy="0 0 0"/>
    <geometry>
      <cylinder radius="0.5" length="1.0"/>
    </geometry>
  </collision>
</link>
"""

        actual_xml_str = etree.tostring(
            link.to_xml(), pretty_print=True)
        self.assertXmlEquivalent(expected_xml, actual_xml_str)

    def test_mesh_link(self):
        self.maxDiff = None
        link = Link(name="link1",
                    visual_origin=Origin(xyz=(0, 0, 0), rpy=(0, 0, 0)),
                    visual_mesh="package://package_name/meshes/mesh.stl",
                    material_name="material_name",
                    material_color="1 0 0 1",
                    collision_origin=Origin(xyz=(0, 0, 0), rpy=(0, 0, 0)),
                    collision_mesh="package://package_name/meshes/mesh.stl",
                    inertial_origin=Origin(xyz=(0, 0, 0), rpy=(0, 0, 0)),
                    inertial_radius="0.5",
                    inertial_length="1.0",
                    inertial_mass="1.0",
                    )

        expected_xml = """
<link name="link1">
  <visual>
    <origin xyz="0 0 0" rpy="0 0 0"/>
    <geometry>
      <mesh filename="package://package_name/meshes/mesh.stl"/>
    </geometry>
    <material name="material_name">
      <color rgba="1 0 0 1"/>
    </material>
  </visual>
  <collision>
    <origin xyz="0 0 0" rpy="0 0 0"/>
    <geometry>
      <mesh filename="package://package_name/meshes/mesh.stl"/>
    </geometry>
  </collision>
  <inertial>
    <mass value="1.0"/>
    <origin xyz="0 0 0" rpy="0 0 0"/>
    <inertia ixx="0.145833275" ixy="0.0" ixz="0.0" iyy="0.145833275" iyz="0.0" izz="0.125000000"/>
  </inertial>
</link>
"""

        actual_xml_str = etree.tostring(
            link.to_xml(), pretty_print=True)
        self.assertXmlEquivalent(expected_xml, actual_xml_str)


if __name__ == "__main__":
    unittest.main()
