# End-effectors

Drop `*.yml` files here with `type: end_effector` entries. They are merged into
the same catalog as arms and become selectable as tools to attach to an arm.

Schema (mirrors arms, plus the `attach` mount contract):

```yaml
robots:
  robotiq_2f85:
    type: end_effector
    display_name: "Robotiq 2F-85"
    description: "Adaptive 2-finger parallel gripper"
    image_path: "resources/graphics/end_effectors/robotiq_2f85.png"
    urdf_package: "robotiq_description"
    urdf_path: "urdf/robotiq_2f85.urdf.xacro"
    xacro_args: ""
    category: "robotiq"           # add the category to categories.yml
    attach:
      mount_frame: "robotiq_base"  # the EE link that connects to the arm
      xyz: [0.0, 0.0, 0.0]         # offset onto the arm's tool_frame
      rpy: [0.0, 0.0, 0.0]
    specifications:
      payload_kg: 5.0
    required_packages:
      - "robotiq_description"
    tags: ["gripper", "parallel", "2finger"]
```

## How attachment works (planned composition)

An assembly = an **arm** + an optional **end_effector**. The arm exposes
`attach.tool_frame` (e.g. `tool0`); the end-effector exposes `attach.mount_frame`
and an `xyz`/`rpy` offset. Composition (to be implemented) generates a small
wrapper xacro that includes both descriptions and links them with a fixed joint:

```
fixed joint: parent = <arm.attach.tool_frame>, child = <ee.attach.mount_frame>,
             origin = ee.attach.xyz / ee.attach.rpy
```

Combinations are **never enumerated** — the chosen arm+EE pair is composed on
demand via the catalog's `get_urdf` path (extended to take an optional
end-effector id). So N arms × M end-effectors stays N + M catalog entries.
