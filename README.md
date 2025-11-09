# ROS2 -> Foxglove Jetson Demo

**A lightweight ROS2 node that simulates robot telemetry (IMU, pose, and battery),
publishes to ROS2 topics, and visualizes the live data stream in Foxglove Studio.**

---


[placeholder]

note to self: do the following to get intellisense to work (for my setup, M4 MBP)
 - open vscode inside the nix dev shell (gotta test to see if this step is actually needed)
 - Run `nix develop`, then go to the pkg folder and build it so colcon will export `compile_commands.json`:
  ``colcon build --cmake-args -DCMAKE_EXPORT_COMPILE_COMMANDS=ON``
 - create a symlink at the root folder for the exported file:
  ``ln -s <pkg folder>/build/compile_commands.json compile_commands.json``
 - I use this `c_cpp_properties.json` file:

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