import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
def generate_launch_description():

    ld = LaunchDescription()

    urdf = os.path.join(get_package_share_directory('ros2zumo_pkg'),
                        'urdf', 'roszumo.urdf')

    rsp = Node(
            package='robot_state_publisher',
            node_executable='robot_state_publisher',
            output='screen', arguments=[urdf])
    ld.add_action(rsp)

    mra = Node(
            package='micro_ros_agent',
            node_executable='micro_ros_agent',
            output='screen', arguments=["udp4 --port 42007 -d"])
    ld.add_action(mra)

    joy_node = Node(
        package="joy",
        executable="joy_node",
        remappings=[
            ("/joy", "/ros2zumo/joy")
        ]        
    )
    ld.add_action(joy_node)
    
#    start_sync_slam_toolbox_node = Node(
#        parameters=[
#          get_package_share_directory("ros2zumo_pkg") + '/config/mapper_params_online_sync.yaml'
#        ],
#        package='slam_toolbox',
#        node_executable='sync_slam_toolbox_node',
#        name='slam_toolbox',
#        output='screen')
        
#    ld.add_action(start_sync_slam_toolbox_node)
        
#    ros1_bridge = Node(
#        package="ros1_bridge",
#        executable="dynamic_bridge"
#    )
#    ld.add_action(ros1_bridge)

#  <node pkg="tf" type="static_transform_publisher" name="tf_02" args="0 0 0.04 0 0 0 base_footprint base_link 100"/>
#  <node pkg="tf" type="static_transform_publisher" name="tf_03" args="0.03 0.02 0.035 0 0 0 base_link imu_link 100"/>
#  <node pkg="tf" type="static_transform_publisher" name="tf_05" args="0.00 0.00 0.11 0 0 0 base_link laser_link 100"/>
#  <node pkg="tf" type="static_transform_publisher" name="tf_06_r"  args="0.0 -0.05 0.0  -1.57 0 0  base_link ranger_r_link 100"/>
#  <node pkg="tf" type="static_transform_publisher" name="tf_06_l"  args="0.0  0.05 0.0   1.57 0 0   base_link ranger_l_link 100"/>
#  <node pkg="tf" type="static_transform_publisher" name="tf_06_fr" args="0.05.0 -0.025 0.0  0 0 0  base_link ranger_fr_link 100"/>
#  <node pkg="tf" type="static_transform_publisher" name="tf_06_fl" args="0.05.0  0.025 0.0  0 0 0  base_link ranger_fl_link 100"/>


#    tf01 = Node(
#        package="tf2_map",
#        executable="static_transform_publisher",
#        args=[0, 0, 0, 0, 0, 0, world, map, 100]
#    )
#    ld.add_action(tf01)
     
    return ld
