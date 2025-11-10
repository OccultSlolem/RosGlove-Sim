# ROS2 -> Foxglove Demo 🦾🤖

**A lightweight ROS2 node that simulates robot telemetry (IMU, pose, and battery),
publishes to ROS2 topics, and visualizes the live data stream in Foxglove Studio.**

---

### The Simulated Bot

Our robot is a very simple "box on wheels" design, equipped with:
- An IMU (Inertial Measurement Unit) for orientation and acceleration data.
- A basic pose estimation system to track its position in space.
- A battery monitor to simulate power levels.
- Wheels for movement.

The robot can be controlled via keyboard inputs to move forward, backward, and turn
through the terminal using the `teleop_twist_keyboard` package.

### Features
- **Lightweight ROS2 Node**: Minimal resource usage, ideal for Jetson Nano and
similar low-power devices.
- **Simulated Telemetry**: Generates realistic IMU, pose, and battery data.
- **Foxglove Studio Integration**: Seamlessly streams data to Foxglove Studio for
real-time visualization and analysis.
- **Keyboard Control**: Use keyboard inputs to control the robot's movement.
- **Customizable Parameters**: Easily adjust simulation parameters to fit your
needs.

### Installation

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/occultslolem/RosGlove-Sim.git
   cd RosGlove-Sim
   ```
2. **Install Dependencies**:
    The best way to do this is probably just to use the provided Dockerfile/compose
    to build a container with everything set up. This repo also supports VSCode
    development containers.

    ```bash
    docker-compose up --build
    ```
    To run in a dev container, make sure you have the [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)
    installed in VSCode, then use the "Reopen in Container" command.

3. **Build the Package**:
    ```bash
    colcon build --symlink-install
    ```
4. **Source the Workspace**:
    ```bash
    source install/setup.bash
    ```
5. **Run the Node**:
    ```bash
    ros2 launch rosglove robot_sim.launch.py
    ```
6. **Control the Robot**:
    In a new terminal, run:
    ```bash
    ros2 run teleop_twist_keyboard teleop_twist_keyboard
    ```
    Use the keyboard to control the robot's movement.
7. **Visualize in Foxglove Studio**:
    - Open Foxglove Studio.
    - Connect to the ROS2 bridge (default port 8765).
    - Add panels to visualize IMU, pose, and battery data.
    - Enjoy real-time telemetry visualization!
  
### Potential Future Improvements
- Add more sensors (e.g., LIDAR, camera).
- Implement obstacle avoidance algorithms.
- Make the robot's movement more complex and realistic.