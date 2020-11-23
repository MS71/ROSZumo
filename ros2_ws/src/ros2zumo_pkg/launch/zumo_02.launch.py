import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node
def generate_launch_description():

    urdf = os.path.join(get_package_share_directory('ros2zumo_pkg'),
                        'urdf', 'roszumo.urdf')
                        
    rviz_config = os.path.join(get_package_share_directory('ros2zumo_pkg'),
                               'config', 'ros2zumo.rviz')
    ld = LaunchDescription()
    joy_node = Node(
        package="joy",
        executable="joy_node",
        remappings=[
            ("/joy", "/ros2zumo/joy")
        ]        
    )
    
    rsp = Node(
            package='robot_state_publisher',
            node_executable='robot_state_publisher',
            output='screen', arguments=[urdf])
            
    rviz = Node(
        package='rviz2',
        node_executable='rviz2',
        arguments=['-d', rviz_config])

    tf_01 = Node(
        package='tf2_ros',
        node_executable='static_transform_publisher',
        name='tf_01',
        arguments=['0 0 0.04 0 0 0 base_footprint base_link 100'])
#ros2 run tf2_ros static_transform_publisher 1 2 3 0.5 0.1 -1.0 foo bar
             
    ld.add_action(joy_node)
    ld.add_action(rviz)
    ld.add_action(rsp)
    return ld
