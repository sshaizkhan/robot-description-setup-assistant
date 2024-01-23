from lxml import etree
from robot_description_setup_assistant.utils.data_classes.elements import Origin


class CylinderInertia:
    def __init__(self, inertial_radius,
                 inertial_length,
                 inertial_mass,
                 inertial_origin: Origin
                 ):

        self.inertial_radius = float(inertial_radius)
        self.inertial_length = float(inertial_length)
        self.inertial_mass = float(inertial_mass)
        self.inertial_origin = inertial_origin

    def calculate_inertia(self):
        # Calculate inertia tensor components for a cylinder
        ixx_iyy = 0.0833333 * self.inertial_mass * \
            (3 * self.inertial_radius**2 + self.inertial_length**2)
        izz = 0.5 * self.inertial_mass * self.inertial_radius**2

        return {
            "ixx": ixx_iyy,
            "ixy": 0.0,
            "ixz": 0.0,
            "iyy": ixx_iyy,
            "iyz": 0.0,
            "izz": izz
        }

    def to_xml(self):
        # Calculate inertia
        inertia = self.calculate_inertia()

        # Create XML representation
        inertial_elem = etree.Element("inertial")
        etree.SubElement(inertial_elem, "mass", value=str(self.inertial_mass))
        etree.SubElement(inertial_elem, "origin",
                         xyz=" ".join(map(str, self.inertial_origin.xyz)),
                         rpy=" ".join(map(str, self.inertial_origin.rpy))
                         )
        etree.SubElement(inertial_elem, "inertia",
                         ixx="{:.9f}".format(inertia["ixx"]),
                         ixy="{:.1f}".format(inertia["ixy"]),
                         ixz="{:.1f}".format(inertia["ixz"]),
                         iyy="{:.9f}".format(inertia["iyy"]),
                         iyz="{:.1f}".format(inertia["iyz"]),
                         izz="{:.9f}".format(inertia["izz"]))

        return inertial_elem
