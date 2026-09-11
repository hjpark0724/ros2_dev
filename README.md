# ROS 2 Jazzy development workspace

This workspace targets **Ubuntu 24.04** and **ROS 2 Jazzy Jalisco**.

## One-time system installation

Run the bootstrap script from the workspace root. It asks for your sudo password,
adds the official ROS 2 apt repository, installs the desktop and development tools,
and initializes `rosdep`.

```bash
./bootstrap_ros2_jazzy.sh
```

## Build and run

Use a new terminal after installation:

```bash
cd /home/hjpark/CLionProjects/ros2_dev
source scripts/setup_ros2.sh
rosdep install --from-paths src --ignore-src -r -y
./scripts/colcon_build.sh
source scripts/setup_ros2.sh
ros2 run ros2_dev_cpp hello_publisher
```

In another terminal, source `scripts/setup_ros2.sh` and inspect the sample topic:

```bash
ros2 topic echo /hello
```

## Service example

After building, start the service server in one terminal:

```bash
source scripts/setup_ros2.sh
ros2 run ros2_service_server_cpp add_two_ints_server
```

In another terminal, source the same workspace and call it with the example client:

```bash
source scripts/setup_ros2.sh
ros2 run ros2_service_client_cpp add_two_ints_client --ros-args -p a:=7 -p b:=35
```

The client prints `Response: 42` and exits. The server keeps running and can
also be exercised directly:

```bash
ros2 service call /add_two_ints example_interfaces/srv/AddTwoInts "{a: 7, b: 35}"
```

## Custom service interface example

The workspace defines `add_two_float_interfaces/srv/AddTwoFloats.srv` and
generates its C++ request and response types during the build. Start the server:

```bash
source scripts/setup_ros2.sh
ros2 run ros2_service_server_cpp add_two_floats_server
```

In another terminal, run the C++ client with parameters:

```bash
source scripts/setup_ros2.sh
ros2 run ros2_service_client_cpp add_two_floats_client --ros-args -p a:=1.5 -p b:=2.25
```

The client prints `Response: 3.750000` and exits. You can also inspect and call
the generated interface directly:

```bash
source scripts/setup_ros2.sh
ros2 interface show add_two_float_interfaces/srv/AddTwoFloats
ros2 service call /add_two_floats add_two_float_interfaces/srv/AddTwoFloats "{a: 1.5, b: 2.25}"
```

The response is `sum: 3.75`.

## CLion

Open the workspace root (`ros2_dev`) in CLion, rather than an individual package.
The root `CMakeLists.txt` discovers packages immediately below `src`, so they share
one CLion project window and each package's targets appear in the target selector.

For CMake configuration, make the ROS 2 Jazzy packages visible to CLion. In
**Settings → Build, Execution, Deployment → CMake**, add this to the selected
profile's CMake options:

```text
-DCMAKE_PREFIX_PATH=/opt/ros/jazzy
```

Use CLion's ordinary build action for an individual CMake target. To build the
complete ROS workspace in the standard way, select the `colcon_build` target and
build it; it runs the same command as:

```bash
./scripts/colcon_build.sh
```

After the first build, source `scripts/setup_ros2.sh` before running ROS commands
from a terminal. It selects `setup.bash` or `setup.zsh` for the active shell.
