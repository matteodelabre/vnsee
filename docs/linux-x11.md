# Setup on Linux with X11

## Setup output

### Intel

When using the Intel X11 driver, virtual outputs `VIRTUAL1` and `VIRTUAL2` are created: you should see them at the bottom of `xrandr`’s output, listed as `disconnected`.

### AMD

AMD drivers seem to lack support for true virtual outputs like Intel. They have a virtual display feature (`virtual_display` in https://dri.freedesktop.org/docs/drm/gpu/amdgpu.html ), but this disables all physical outputs by design, so it doesn't help in this use case.

### Nvidia

Unknown.


### Generic workaround

One workaround is to use an existing output that isn't plugged in. This might lead to weird issues, especially if corresponding GPU output is suddenly connected. 



## Setup the display mode

Create a new mode compatible with the tablet’s resolution:

```
xrandr --newmode 1408x1872 $(gtf 1408 1872 60 | tail -n2 | head -n1 | tr -s ' ' | cut -d' ' -f4-)
```


## Setup the output

### Laptop with Intel GPU

on a modern laptop with an Intel GPU the internal display will most likely be called `eDP1` and the output `VIRTUAL1` should be available for the VNC server 

First add the mode to the `VIRTUAL1` output
```sh
xrandr --addmode VIRTUAL1 1408x1872
```


Enable and place the `VIRTUAL1` output through your usual dual screen configuration program such as arandr, GNOME’s settings panel or KDE setting. Alternatively you can manually configure it with:

```sh
xrandr --output VIRTUAL1 --mode 1408x1872 --right-of eDP1
```

### Using the generic workaround

This example reuses a disconnected HDMI port that appears as the output `HDMI-1` on a dual monitor system with `DP-1` and `DP-2` as regular monitors. Using an output that is actually connected will most likely cause issues with the mode that was added, so be sure that the output isn't currently in use.

#### Get unused output

```sh
$ xrandr | grep disconnected
DP-3 disconnected (normal left inverted right x axis y axis)
HDMI-1 disconnected (normal left inverted right x axis y axis)
```

#### Setup mode

Add the mode to the desired output:

```sh
xrandr --addmode HDMI-1 1408x1872
```

#### Setup screen

GUI tools like `lxrandr` or `arandr` will not recognize or show the output because it is still disconnected. Manual `xrandr` commands still work.

To setup the tablet as a new screen below an existing monitor, use:

```sh
xrandr --output HDMI-1 --mode 1408x1872 --below DP-1
```


## Start the VNC server

Any VNC server can be used for this task.
We recommend x11vnc, which can be launched using the following command line on an Intel laptop setup:

```sh
x11vnc -repeat -forever -nocursor -allow 10.11.99.1 -clip $(xrandr | awk '/VIRTUAL1 connected/{print $3}')
```

For the generic workaround, you need to get the position and resolution for the output of your choice, e.g.:

```sh
x11vnc -repeat -forever -nocursor -allow 10.11.99.1 -clip $(xrandr | awk '/HMDI-1 disconnected/{print $3}')
```
Note that the output is still expected to appear as `disconnected`!

### Options

Flag         | Description
----         | -----------
`-repeat`    | If omitted, your computer keys will not repeat when you hold them
`-forever`   | Keep the server alive even after the first client has disconnected
`-nocursor`  | Hide the mouse pointer when it is on the tablet’s screen
`-allow`     | **Security:** Only allow connections through the USB interface
`-clip`      | Restrict the display to the `VIRTUAL1` output

Here are some additional flags that might be of interest:

Flag         | Description
----         | -----------
`-rotate xy` | Use to flip the screen upside down

## Start rmvncclient

Finally, start rmvncclient using SSH.

```sh
ssh root@10.11.99.1 "systemctl stop xochitl && ./rmvncclient; systemctl start xochitl"
```

**Note:** If you get a message saying that the `Server uses an unsupported resolution`, you did not configure your screen correctly.
Please make sure that the `VIRTUAL1` output is enabled.
