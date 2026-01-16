from launch import LaunchDescription
from launch.substitutions import Command, FindExecutable, PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare
from launch_ros.actions import Node

def generate_launch_description():
    pkg_share = FindPackageShare('rosglove')
    urdf = PathJoinSubstitution([pkg_share, 'urdf', 'boxbot.urdf.xacro'])

    robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        name='robot_state_publisher',
        output='screen',
        parameters=[{
            'robot_description': Command([FindExecutable(name='xacro'), ' ', urdf])
        }]
    )

    sim = Node(
        package='rosglove',
        executable='robot_simulator',
        name='robot_simulator',
        output='screen',
        parameters=[{
            'wheel_radius': 0.033,
            'wheel_separation': 0.16,
            'update_rate_hz': 50.0,
            'cmd_timeout_sec': 0.5,
            'base_frame': 'base_link',
            'odom_frame': 'odom',
            'imu_frame': 'imu_link',
            'battery_start_percent': 100.0,
            'battery_depletion_per_sec': 0.1,
            'base_temperature_c': 20.0
        }]
    )

    bridge = Node(
        package='foxglove_bridge',
        executable='foxglove_bridge',
        name='foxglove_bridge',
        output='screen',
        parameters=[{
            'port': 8765,  # Default WebSocket port; change if needed
            'address': '0.0.0.0'
        }]
    )

    ota_update = Node(
        package='rosglove',
        executable='ota_update_node',
        name='ota_update_node',
        output='screen',
        parameters=[{
            'current_version': '1.0.0',
            'download_duration_sec': 10.0,
            'install_duration_sec': 5.0,
            'failure_probability': 0.1,
            'update_rate_hz': 10.0
        }]
    )

    return LaunchDescription([
        robot_state_publisher,
        sim,
        bridge,
        ota_update
    ])