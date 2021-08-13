# Setup as a Second Screen on Linux with X11

## Find a Suitable Output

The first step is to create a new space on your computer on which applications can be launched and that can be mirrored on the reMarkable through VNC, without being visible on your computer.

It is assumed that you are using the [RandR X extension](https://www.x.org/releases/current/doc/randrproto/randrproto.txt) to arrange the different screens of your computer, which is likely the case with any recent Linux distribution.
With RandR, each screen connected to your computer (called an _output_) sees a subregion of a larger shared image (perhaps confusingly called a _screen_).
Even though the overall size of that shared image can be controlled manually—which could allow us to allocate some extra space outside of the other outputs for use as a second screen—this size is usually automatically calculated to cover the combined sizes of all available outputs, so the most robust way to allocate that space is to create a _virtual_ output.
The steps required to enable those _virtual_ outputs, which are treated like any other output but are not visible, vary depending on the driver that you use.

### Intel

When using the Intel X11 driver, virtual outputs called `VIRTUAL1`, `VIRTUAL2`, … should be available.
You should see them at the bottom of `xrandr`’s output, listed as `disconnected`.
If not, you need to configure the driver’s `VirtualHeads` option appropriately; if you don’t already have an existing configuration file for the Intel driver, create one at `/etc/X11/xorg.conf.d/20-intel.conf` with the following contents:

```conf
Section "Device"
    Identifier "intelgpu0"
    Driver "intel"
    Option "VirtualHeads" "1"
EndSection
```

If you changed the driver configuration, restart your X session before proceeding.

### AMD

AMD drivers seem to lack support for true virtual outputs like Intel.
They have a virtual display feature (see the `virtual_display` parameter in the [AMDgpu driver documentation](https://dri.freedesktop.org/docs/drm/gpu/amdgpu.html)), but this disables all physical outputs by design, so it doesn't help in this use case.

### Nvidia

Unknown.

### Generic Workaround

One workaround is to use any existing output that isn't plugged in.
This might lead to weird issues, especially if the corresponding screen is later connected.
To list all disconnected outputs, use the following command:

```console
$ xrandr | grep disconnected
DP-3 disconnected (normal left inverted right x axis y axis)
HDMI-1 disconnected (normal left inverted right x axis y axis)
```

Here, you may use either of the `DP-3` or `HDMI-1` outputs for the following instructions.

## Create a Compatible Display Mode

Create a new mode compatible with the tablet’s resolution:

```console
$ xrandr --newmode 1404x1872 0 1404 1404 1404 1404 1872 1872 1872 1872
```

## Setup the Output

_In the following, replace `OUTPUTNAME` by the name of the output you want to mirror on the reMarkable, as determined in the first section._

First add the display mode to the set output:

```console
$ xrandr --addmode OUTPUTNAME 1404x1872
```

Enable and place the set output through your usual dual screen configuration program such as arandr, GNOME’s settings panel or KDE settings.
When using the generic workaround, the output you chose will not appear in those programs, and you need to use `xrandr` as follows:

```console
$ xrandr --output OUTPUTNAME --mode 1404x1872 --right-of MAINOUTPUT
```

Where `MAINOUTPUT` is the name of your main screen to the right of which you want to place the second screen.
You may replace `--right-of` by `--left-of`, `--above`, or `--below`.
Ignore any error saying `xrandr: Configure crtc X failed`.

## Start the VNC Server

_In the following, replace `OUTPUTNAME` by the name of the output you want to mirror on the reMarkable, as determined in the first section._

Any VNC server can be used for this task.
We recommend x11vnc, which can be launched using the following command line:

```console
$ x11vnc -repeat -forever -nocursor -allow 10.11.99.1 -nopw -clip $(xrandr | perl -n -e'/OUTPUTNAME .*?(\d+x\d+\+\d+\+\d+)/ && print $1')
```

### Options

Flag         | Description
----         | -----------
`-repeat`    | If omitted, your computer keys will not repeat when you hold them
`-forever`   | Keep the server alive even after the first client has disconnected
`-nocursor`  | Hide the mouse pointer when it is on the tablet’s screen
`-allow`     | **Security:** Only allow connections through the local USB interface
`-nopw`      | Hide the banner saying that running x11vnc without a password is insecure. It is safe in this case because we’re only accepting connections through the local USB interface
`-clip`      | Restrict the display to the set output

Here are some additional flags that might be of interest:

Flag         | Description
----         | -----------
`-rotate xy` | Use to flip the screen upside down

## Start VNSee

If you installed VNSee through [Toltec](https://toltec-dev.org) and you’re using a launcher such as [Oxide](https://github.com/Eeems/oxide) or [remux](https://github.com/rmkit-dev/rmkit/tree/master/src/remux) (which is the recommended setup), VNSee should show up in the list of available apps on the tablet.
Starting the VNSee app will bring up a screen listing the available VNC servers, in which you should see a server listening on `10.11.99.2:5900`.
Tap on that server and you should see your computer’s screen appear on the reMarkable after a few seconds.

Otherwise, you can also start VNSee manually through SSH. Don’t forget to stop the main UI app first (`systemctl stop xochitl`) and start it again afterwards (`systemctl start xochitl`).
