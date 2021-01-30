# Setup as a second screen on Linux with X11

## Find a suitable output

The first step is to create a new space on your computer on which applications can be launched and that can be mirrored on the reMarkable through VNC, without being visible on your computer.

This guide assumes you have sway running properly with only one display connected (VNC will be the second screen). The guide will probably work if you have more than one screen running, too, but this has not been tested.

## Create a virtual output and set the correct dimensions

Use swaymsg to control your running instance of sway. 

In a terminal, create a new virtual output:
```sh
$ swaymsg create_output
```

Then get the name of the output (in this example HEADLESS-1). To list all outputs, run:
```sh
$ swaymsg -t get_outputs
```

## Create a compatible display mode

To set the dimensions of the output, run (replace HEADLESS-1 with your output if necessary):

```sh
$ swaymsg output HEADLESS-1 mode 1404x1872
```

## Start the VNC server

_In the following, replace `HEADLESS-1` by the name of the output you want to mirror on the reMarkable, as determined in the first section._

This guide assumes that you use wayvnc as a vnc server. It requires you to specify an address to listen to (otherwise it may listen on your WiFi connection and vnsee cannot connect). Either enter the IP-Adress your Linux box received from the tablet (10.11.99.X) or listen anywhere with 0.0.0.0. In this example we will listen anywhere:

```sh
$ wayvnc --output=HEADLESS-1 0.0.0.0
```

This will create a new workspace in sway under the lowest number that is not assigned to an output yet (the lowest number you cannot see in the workspace overview). To see it, connect the VNC client.

## Start VNSee

Finally, start VNSee using SSH.

```sh
$ ssh root@10.11.99.1 "systemctl stop xochitl && ./vnsee; systemctl start xochitl"
```

**Note:** If you get a message saying that the `Server uses an unsupported resolution`, you did not configure your screen correctly.
Please make sure that the chosen output is enabled.
