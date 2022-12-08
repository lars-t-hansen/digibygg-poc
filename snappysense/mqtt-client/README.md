This program is derived from the AWS embedded C IOT SDK,
https://github.com/aws/aws-iot-device-sdk-embedded-C, version 202108.00.  It is a variant of the
demos/mqtt/mqtt_demo_mutual_auth demo program.

Generally speaking, the AWS IoT SDK only works on Linux.

**BUILDING**

To build snappysense, you need to check out, configure, and build the libraries for the AWS IoT
embedded C SDK somewhere else in your directory tree.  Let $AWS be the absolute path to the AWS IoT
embedded C SDK root directory.  Then

```
   cd $AWS
   cmake -S . -B build
   cd build
   make
   find . -name '*.so'  <--- this should list a bunch of libraries
```

Then in the CMakeLists.txt in this directory, set the value of AWS to reflect the location of the
AWS directory.  Then:

```
   cmake -S . -B build
   cd build
   make
   cd ..
```

The final executable is build/snappysense.

**RUNNING**

Now update snappysense.cfg in this directory to reflect the values for your device: the device ID
and class, the paths to its secrets, and so on - pretty much every value in that file apart from the
MQTT port should be changed.

To run, the libraries built by the build process must be in a suitable path, I usually do

```
   LD_LIBRARY_PATH=$AWS/build/lib build/snappysense
```

**ABOUT THE SOURCES**

In this directory:

*  mqtt-client.c is a generic mqtt client derived from the AWS SDK, adapted to our needs.
*  core_mqtt_config.h is a header file for the mqtt client, this should probably be cleaned up, it's
   not obvious that it's needed.
*  configfile.{c,h} is code to read the snappysense config file.
*  sensors.c read the sensors and work the actuators attached to the device.
*  snappysense.{c,h} has the main program and logic for driving the mqtt client, reading sensors,
   working actuators, and similar.
