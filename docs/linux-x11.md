# Setup on Linux with X11

## Enable virtual outputs

When using the Intel X11 driver, virtual outputs `VIRTUAL1` and `VIRTUAL2` are created: you should see them at the bottom of `xrandr`’s output, listed as `disconnected`.
No reliable workaround is yet known for other drivers.

## Configure a virtual second output

In the following, it is assumed that we want to configure the `VIRTUAL1` output for use on the reMarkable.

Create a new mode compatible with the tablet’s resolution and add this mode to the `VIRTUAL1` output.

```sh
xrandr --newmode 1408x1872 $(gtf 1408 1872 60 | tail -n2 | head -n1 | tr -s ' ' | cut -d' ' -f4-)
xrandr --addmode VIRTUAL1 1408x1872
```

Enable and place the `VIRTUAL1` output through your usual dual screen configuration program.
For example, using `xrandr`, if your main output is called `eDP1` you can do:

```sh
xrandr --output VIRTUAL1 --mode 1408x1872 --right-of eDP1
```

You can also use any other screen configuration tool, such as arandr, GNOME’s settings panel or KDE settings.

## Start the VNC server

Any VNC server can be used for this task.
We recommend x11vnc, which can be launched using the following command line:

```sh
x11vnc -repeat -forever -nocursor -allow 10.11.99.1 -clip $(xrandr | awk '/VIRTUAL1 connected/{print $3}')
```

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
