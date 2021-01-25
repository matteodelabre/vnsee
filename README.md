# VNSee

[VNC](https://en.wikipedia.org/wiki/Virtual_Network_Computing) client for the [reMarkable tablet](https://remarkable.com) allowing you to use the device as a second screen.\
(Previously called _rmvncclient_.)

<img alt="Illustration of a reMarkable table connected to a computer, showing half of a terminal window through its E-Ink screen" src="media/setup.gif" width="700">

## Disclaimer

This project is not affiliated to, nor endorsed by, [reMarkable AS](https://remarkable.com/).\
**I assume no responsiblitiy for any damage done to your device due to the use of this software.**

## Background

VNC is a desktop-sharing system that enables a client to see the screen of another computer and act on it remotely.
It relies on a well-established protocol supporting multiple platforms including Windows, Linux and Android.
The reMarkable is a writer tablet featuring an E-Ink display, a Wacom pen digitizer and a touchscreen, that can be used for reading, sketching or note-taking.
It runs a fully open, Linux-based, system.

VNSee brings both worlds together by allowing the tablet to connect to a remote VNC server, show the remote screen on its E-Ink display and interact with it through the pen digitizer and touchscreen.
This can effectively turn the tablet into a second screen for your computer.
Applications include reading web-based content, typing documents, drawing, or previewing [LaTeX documents](https://www.latex-project.org/) as you compose them.

## Install on the reMarkable

Grab the latest build from the [releases page](https://github.com/matteodelabre/vnsee/releases) (or build the software yourself by following the [build guide](docs/build.md)), then copy the `vnsee` executable to the tablet [using SSH](https://remarkablewiki.com/tech/ssh).

This VNC client is compatible with VNC servers that are capable of sending pixels in the RGB565 format and that use a resolution compatible with reMarkable’s screen size (1408 × 1872 pixels).
It has successfully been tested with [x11vnc](https://github.com/LibVNC/x11vnc).
When trying to connect to an incompatible server, the program will report details of the problem then exit.

## Setup as a second screen

Although this client can connect to any compatible VNC server, the most common scenario is using the tablet as a second screen for the computer it is attached to.
For this scenario, you will need to configure your system to create a virtual second screen and then start a VNC server on your computer that the tablet can connect to.

The details are specific to the operating system you’re using:

* [Linux with X11](docs/second-screen/linux-x11.md)
* Linux with Wayland (Help wanted!)
* macOS (Help wanted!)
* Windows (Help wanted!)

## Using the client

You can quit VNSee at any time using the “Power” button (the one above the screen).
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
        <strong>Tap</strong> → Left click<br>
        <strong>Long press</strong> → Right click
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
    <td></td>
</tr>
<tr>
    <th>Pen</th>
    <td align="center" colspan="3">
        Pen input will be converted to mouse click and drag.
    </td>
</tr>
</table>

While the client is running, frames will be displayed on the tablet’s screen as they are received from the server.
Due to the properties of E-Ink, there will be some extra latency (up to 1s) between the time of a change on the computer and the moment it appears on the screen.
On dark background apps, there will be some ghosting on the screen: use the “Home” button (the one in the middle of the button row below the screen) to force a refresh and clear those artifacts out.

## Technologies

This client was built in C++ using [libvncserver](https://github.com/LibVNC/libvncserver), which implements the [RFB protocol](https://tools.ietf.org/html/rfc6143) behind the VNC system.

## Acknowledgments

Many thanks to:

- [libremarkable](https://github.com/canselcik/libremarkable) and [FBInk](https://github.com/NiLuJe/FBInk), on which input/output handling in this client is based.
- [Damien Challet](https://github.com/damienchallet) and [Qwertystop](https://news.ycombinator.com/item?id=13115739) for providing the inspiration of a VNC client for the reMarkable.
- The [Discord developer community](https://discord.gg/JSSGnFY) for providing initial feedback and testing.
- [Florian Magin](https://github.com/fmagin) for contributing a driver-generic way of setting up Linux/X11 systems.
- [@asmanur](https://github.com/asmanur) for improving the repaint latency.
- [@mhhf](https://github.com/mhhf) for helping with TigerVNC compatibility.

[Martin Sandsmark](https://github.com/sandsmark) also built [a VNC client for the reMarkable](https://github.com/sandsmark/revncable) which only depends on Qt.

## License

This work is licensed under the GPL v3.
