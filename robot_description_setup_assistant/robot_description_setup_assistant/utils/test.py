import os

# get the path of the current file
current_file_path = os.path.abspath(__file__)

# get the directory two levels above the current file
two_levels_up_dir = os.path.abspath(
    os.path.join(
        current_file_path,
        "..",
        "..",
        "..",
        "..",
        "robot_description_resources",
        "config",
        "ur5",
        "default_kinematics.yaml",
    )
)

print(two_levels_up_dir)
