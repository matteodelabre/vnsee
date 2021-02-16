# Setup as a Second Screen on Linux with Sway

## Create a Headless Output

First up we’ll create a _headless_ output, i.e. a space to run applications that’s not visible on any of your screens and that can be made accessible over a VNC connection.
Run `swaymsg create_output` in a terminal to create a new output called `HEADLESS-1`.
This output will need to be recreated each time you start Sway, so you might want to add the command in your session startup script.

## Setup the Output

Set the size of the new output to exactly 1408x1872 and place it where you’d like relative to your other (real) screens.
This can be done interactively with [wdisplays](https://github.com/cyclopsian/wdisplays) or using the [command line](https://man.archlinux.org/man/sway-output.5).
You can make the output configuration permanent by editing your Sway config file (located in `~/.config/sway/config`).

## Start the VNC Server

Any Wayland-compatible VNC server can be used for this task.
We recommend [wayvnc](https://github.com/any1/wayvnc) which you can start with the following command line:

```console
$ wayvnc --output=HEADLESS-1 --max-fps=10 10.11.99.2 5900
```

It may happen that your computer isn’t assigned the 10.11.99.2 IP by the reMarkable DHCP server. In this case, replace this IP with the one that’s been assigned.

## Start VNSee

Finally, start VNSee using SSH.

```console
$ ssh root@10.11.99.1 "systemctl stop xochitl && ./vnsee; systemctl start xochitl"
```

**Note:** If you get a message saying that the `Server uses an unsupported resolution`, you did not set the right size for the headless output.
