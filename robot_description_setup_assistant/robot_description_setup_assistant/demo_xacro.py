# #!/usr/bin/env python


from lxml import etree

# Define the Xacro namespace
NS_XACRO = "http://www.ros.org/wiki/xacro"

# Create the root element
root:list = etree.Element("robot", name="my_robot", nsmap={None: NS_XACRO})

material: list = etree.Element("material", name="blue")
color:list = etree.Element("color", rgba="0 0 0.8 1")
material.append(color)
root.append(material)

material: list = etree.Element("material", name="white")
color:list = etree.Element("color", rgba="1 1 1 1")
material.append(color)
root.append(material)

# Create the link element
link:list = etree.Element("link", name="my_link")

# Create the visual element and add it to the link
visual:list = etree.Element("visual")
origin: list = etree.Element("origin", rpy="0 0 3.141592653589793", xyz="0 0 0")
geometry: list = etree.Element("geometry")
cylinder: list = etree.Element("cylinder", length="0.6", radius="0.2")
geometry.append(cylinder)
visual.append(origin)
visual.append(geometry)
link.append(visual)

# Create the collision element and add it to the link
collision:list = etree.Element("collision")
origin: list = etree.Element("origin", rpy="0 0 3.141592653589793", xyz="0 0 0")
geometry: list = etree.Element("geometry")
cylinder: list = etree.Element("cylinder", length="0.6", radius="0.2")
geometry.append(cylinder)
collision.append(origin)
collision.append(geometry)
link.append(collision)

# Add the link to the robot
root.append(link)

# Write the Xacro file to disk
with open("/Users/shah/Dev/dev_ws/ros2_ws/src/dev_app/robot-description-setup-assistant/robot_description_setup_assistant/generated_xacro/my_custom_robot.xacro", "wb") as f:
    f.write(etree.tostring(root, xml_declaration=True, encoding="UTF-8", pretty_print=True))
