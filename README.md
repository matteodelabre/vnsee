# rmvncclient

A simple [VNC](https://en.wikipedia.org/wiki/Virtual_Network_Computing) client for the [reMarkable tablet](https://remarkable.com) allowing you to turn the device into an external screen for your computer.

VNC is a desktop-sharing system that enables a client to see the screen of another computer and act on it remotely.
It is a well-established protocol supporting multiple platforms including Windows, Linux and Android.

The reMarkable is a writer tablet featuring an E-Ink display, Wacom pen digitizer and a touchscreen, that can be used for reading, sketching or note-taking.
It runs Linux and is open to hacking.

rmvncclient brings both worlds together by allowing the tablet to connect to a remote VNC server, show the computer’s screen on its E-Ink display and interact with it through the touchscreen.
This effectively turns the tablet into an external screen for your computer.

Applications include reading web-based documents, writing inside an editor or previewing your [LaTeX documents](https://www.latex-project.org/) as you compose them.

## Installation

Grab the latest build from the releases page (or build the software yourself by following the [build guide](BUILD.md)), then copy the `rmvncclient` executable to the tablet by [using SSH](https://remarkablewiki.com/tech/ssh).

## Running

You will first need a VNC server to connect to that is configured for the reMarkable resolution (1408 × 1872 pixels).
This is an important limitation: if the server sends an incompatible resolution, this client will exit immediately because it does no image processing for simplicity and efficiency reasons.

A common scenario is using the tablet as an external screen for the computer it is attached to through USB (or via Wi-Fi).
For this scenario, you will need to start a VNC server on your computer.

### Linux (X11 with Xrandr)

Create a new mode compatible with the tablet’s resolution and enable the VIRTUAL1 output with this mode.

```sh
xrandr --newmode 1408x1872 $(gtf 1408 1872 60 | tail -n2 | head -n1 | tr -s ' ' | cut -d' ' -f4-)
xrandr --addmode VIRTUAL1 1408x1872
```

Start the x11vnc server (you will need to install this package first if needed).

```sh
x11vnc -repeat -forever -clip $(xrandr | awk '/VIRTUAL1 connected/{print $3}') -nocursor
```

> **Flip the screen upside down**
> 
> ```sh
> x11vnc -repeat -forever -clip $(xrandr | awk '/VIRTUAL1 connected/{print $3}') -nocursor -rotate xy
> ```

Finally, start rmvncclient using SSH.

```sh
ssh "root@10.11.99.1" "./rmvncclient"
```

## Related

- <https://github.com/damienchallet/vnc-remarkable>
- <https://news.ycombinator.com/item?id=13115739>
