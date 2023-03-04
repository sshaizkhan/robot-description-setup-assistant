import os
from glob import glob
from setuptools import setup

package_name = 'robot_description_setup_assistant'

setup(
    name=package_name,
    version='0.0.0',
    packages=[package_name],
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        (os.path.join('share', package_name), glob('launch/*launch.[pxy][yma]*'))
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='Shahwaz Khan',
    maintainer_email='sshaizkhan@gmail.com',
    description='PyQt5 based gui to create robot descriptions with custom end-effector tools',
    license='BSD-Clause-3',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
        ],
    },
)
