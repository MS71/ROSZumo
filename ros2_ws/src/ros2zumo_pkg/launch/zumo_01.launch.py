import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node
def generate_launch_description():

    ld = LaunchDescription()
    joy_node = Node(
        package="joy",
        executable="joy_node",
        remappings=[
            ("/joy", "/ros2zumo/joy")
        ]        
    )
             
    ld.add_action(joy_node)
    return ld
