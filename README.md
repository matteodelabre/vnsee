# rmvncclient

A simple [VNC](https://en.wikipedia.org/wiki/Virtual_Network_Computing) client for the [reMarkable tablet](https://remarkable.com) allowing you to turn the device into an external screen for your computer.

<img alt="Illustration of a reMarkable table connected to a computer, showing half of a terminal window through its E-Ink screen" src="media/setup.svg" width="700">

* [Disclaimer](#disclaimer)
* [Background](#background)
* [Installing](#installing)
* [Running](#running)
    - [Linux (X11 with xrandr)](#linux-x11-with-xrandr)
    - [Linux (Wayland) — _Help wanted!_](#linux-wayland)
    - [Windows — _Help wanted!_](#windows)
    - [macOS — _Help wanted!_](#macos)
* [Using](#using)
* [Technologies](#technologies)
* [Related work](#related-work)
* [License](#license)

## Disclaimer

This project is not affiliated to, nor endorsed by, [reMarkable AS](https://remarkable.com/).
**I am not responsible for any damage done to your device due to the use of this software.**

## Background

VNC is a desktop-sharing system that enables a client to see the screen of another computer and act on it remotely.
It is a well-established protocol supporting multiple platforms including Windows, Linux and Android.
The reMarkable is a writer tablet featuring an E-Ink display, Wacom pen digitizer and a touchscreen, that can be used for reading, sketching or note-taking.
It runs Linux and is open to hacking.

rmvncclient brings both worlds together by allowing the tablet to connect to a remote VNC server, show the computer’s screen on its E-Ink display and interact with it through the touchscreen.
This effectively turns the tablet into an external screen for your computer.
Applications include reading web-based documents, writing inside an editor or previewing your [LaTeX documents](https://www.latex-project.org/) as you compose them.

## Installing

Grab the latest build from the [releases page](https://github.com/matteodelabre/rmvncclient/releases) (or build the software yourself by following the [build guide](BUILD.md)), then copy the `rmvncclient` executable to the tablet by [using SSH](https://remarkablewiki.com/tech/ssh).

## Running

You will first need a VNC server to connect to that is configured for the reMarkable resolution (1408 × 1872 pixels).
This is an important limitation: if the server sends an incompatible resolution, this client will exit immediately because it does no image processing for simplicity and efficiency reasons.

A common scenario is using the tablet as an external screen for the computer it is attached to through USB (or via Wi-Fi).
For this scenario, you will need to start a VNC server on your computer.

### Linux (X11 with xrandr)

Create a new mode compatible with the tablet’s resolution and add this mode to the `VIRTUAL1` output.

```sh
xrandr --newmode 1408x1872 $(gtf 1408 1872 60 | tail -n2 | head -n1 | tr -s ' ' | cut -d' ' -f4-)
xrandr --addmode VIRTUAL1 1408x1872
```

Enable and arrange the `VIRTUAL1` output through your usual dual screen configuration program, for example GNOME’s settings, arandr or xrandr directly.
Then, start the x11vnc server (you will need to install this package first if needed).

```sh
x11vnc -repeat -forever -nocursor -allow 10.11.99.1 -clip $(xrandr | awk '/VIRTUAL1 connected/{print $3}')
```

> _Alternative: Flip the screen upside down_
>
> ```sh
> x11vnc -rotate xy -repeat -forever -nocursor -allow 10.11.99.1 -clip $(xrandr | awk '/VIRTUAL1 connected/{print $3}')
> ```

Finally, start rmvncclient using SSH.

```sh
ssh root@10.11.99.1 "systemctl stop xochitl && ./rmvncclient; systemctl start xochitl"
```

**Note:** If you get a message saying that the `Server uses an unsupported resolution`, you did not configure your screen correctly. Please make sure the `VIRTUAL1` output is enabled.

### Linux (Wayland)

(Help wanted!)

### Windows

(Help wanted!)

### macOS

(Help wanted!)

## Using

You can quit rmvncclient at any time using the “Power” button (the one above the screen).

While the client is running, frames will be displayed on the tablet’s screen as they are received from the server.
Due to the properties of E-Ink, there will be some extra latency (up to 1s) between the time of a change on the computer and the moment it appears on the screen.
On dark background apps, there will be some ghosting on the screen: use the “Home” button (the one in the middle of the button row below the screen) to force a refresh and clear those artifacts out.

Activity on the touchscreen will be translated to mouse interactions following the mapping in the table below.

<table>
<tr>
<th colspan="4">
    Interactions
</th>
</tr>
<tr>
    <th>Touch<br>screen</th>
    <td align="center">
        <img src="media/tap.svg" width="200" alt=""><br>
        <strong>Tap</strong><br>
        Left click
    </td>
    <td align="center">
        <img src="media/scroll-x.svg" width="200" alt=""><br>
        <strong>Horizontal swipe</strong><br>
        Horizontal scroll
    </td>
    <td align="center">
        <img src="media/scroll-y.svg" width="200" alt=""><br>
        <strong>Vertical swipe</strong><br>
        Vertical scroll
    </td>
</tr>
<tr>
    <th>Buttons</th>
    <td align="center">
        <img src="media/home.svg" width="200" alt=""><br>
        <strong>Home button</strong><br>
        Force refresh
    </td>
    <td align="center">
        <img src="media/power.svg" width="200" alt=""><br>
        <strong>Power button</strong><br>
        Quit app
    </td>
</tr>
</table>

## Technologies

This client is built in C++ using [libvncserver](https://github.com/LibVNC/libvncserver), which implements the [RFB protocol](https://tools.ietf.org/html/rfc6143) behind the VNC system.

## Related work

- [A Hacker News comment requesting a VNC client for the reMarkable.](https://news.ycombinator.com/item?id=13115739)
- [Another VNC client built by @sandsmark which does not depend on libvncserver.](https://github.com/sandsmark/revncable)
- [A currently-unfinished previous attempt at building a VNC client for the tablet.](https://github.com/damienchallet/vnc-remarkable)
- [libremarkable](https://github.com/canselcik/libremarkable), which was used as the basis for some input/output handling.

## License

This work is licensed under the GPL v3.
