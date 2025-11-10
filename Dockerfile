FROM ros:jazzy-ros-base

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y\
    git \
    python3-colcon-common-extensions \
    ros-jazzy-tf2-ros \
    ros-jazzy-nav-msgs \
    ros-jazzy-geometry-msgs \
    ros-jazzy-tf2-geometry-msgs \
    ros-jazzy-xacro \
    ros-jazzy-teleop-twist-keyboard \
    ros-jazzy-foxglove-bridge \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY rosglove/ ./src/rosglove

# Build workspace
RUN /bin/bash -c "source /opt/ros/jazzy/setup.bash && colcon build"

# Source the setup script
RUN echo "source /opt/ros/jazzy/setup.bash" >> ~/.bashrc
RUN echo "source /workspace/rosglove/install/setup.bash" >> ~/.bashrc

CMD ["/bin/bash"]