# ROS2 -> Foxglove Jetson Demo

**A lightweight ROS2 node that simulates robot telemetry (IMU, pose, and battery),
publishes to ROS2 topics, and visualizes the live data stream in Foxglove Studio.**

---

### Our simulated robot

For the purposes of this repo, we will simulate a simple "box-on-wheels" type robot
that can collect the following data from its sensors:
- TBD

### Development

This repo uses **nix** to allow for an easily reproducible development environment. After
cloning the repo, all you need to do is run `nix develop` and it will download the
necessary dependencies and boot you into a shell you can run the project from.

**Foxglove connection**

Placeholder

**VSCode Setup (macOS)**

These were the steps I had to run to get intellisense working on my setup (M4 MacBook Pro, macOS Tahoe 26.0.1)
 - Run `code .` from inside the nix dev shell (gotta test to see if this step is actually needed)
 - From inside the nix shell, cd to the pkg folder and build it so colcon will export `compile_commands.json`, like so:
  ``colcon build --cmake-args -DCMAKE_EXPORT_COMPILE_COMMANDS=ON``
 - Create a symlink at the root folder for the exported file:
  ``ln -s <pkg folder>/build/compile_commands.json compile_commands.json``
 - I use this `.vscode/c_cpp_properties.json` file:

```json
{
  "configurations": [
    {
      "name": "Default",
      "compileCommands": "${workspaceFolder}/compile_commands.json",
      "intelliSenseMode": "gcc-x64"
    }
  ],
  "version": 4
}
```

Nix on macOS is weird because of it can't generate a `/nix` file due to macOS making the root directory read-only since Catalina. The typical Nix install will instead create a separate APFS volume, so this method is the easiest way (that I know of) to get the relevant files and directories aware to C++ intellisense.

---

#### Special Thanks

- Ben Wolsieffer's [nix-ros-overlay](https://github.com/lopsided98/nix-ros-overlay)