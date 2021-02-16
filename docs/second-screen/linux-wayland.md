**Required Package Installation**

First you need to ensure that `wayvnc` is installed.
This requires installation of both the package `aml` and `neatvnc` from the arch user repository or wherever you get your packages.
After installing these packages `wayvnc` should install successfuly.

**Create A Suitable Output**
First up create a mode called `HEADLESS-1` The output must be named this in order to work successfuly under Sway

Add the following code to the config file for sway at the very bottom. 

This config file can be found at `~/.config/sway/config`

```
output HEADLESS-1 {
	pos RIGHTMOST,0
	mode 1408x1872
	bg COLOR solid_color
}
```
This creates a new headless output name at position [RIGHTMOST, 0] if you have multiple monitors take the position of the rightmost one and add its width to get the rightmost position where you should position your new display.
Replace RIGHTMOST with this calculated rightmost point.
Replace COLOR with the 9 digit color code for a color.

Outputs from sway can be found with the following command.
```
> swaymsg -t get_outputs
Output eDP-1 'Unknown 0x0791 0x00000000' (focused)
  Current mode: 1920x1080 @ 60.012001 Hz
  Position: 0,0
```
In this example I would replace RIGHTMOST with 0 + 1920 because this screen starts at 0 and has a width of 1920 meaning that our RIGHTMOST point would be [1920, 0] to place the screen directly alongside.

Next restart Sway and run the command

```
swaymsg create_output HEADLESS-1

```
This initializes the headless output for usage with wayvnc

**Starting VNC Server**
Next start your vnc server with the following command

```

wayvnc --output=HEADLESS-1 --max-fps=10 10.11.99.2 5900 & 

```
This binds the server to the ip on which the remarkable is connected via usb to the normal port that is used for VNC connectoins.

Finally log on to the remarkable and run the vnsee command


**Connecting With Remarkable**
```
ssh root@10.11.99.1
systemctl stop xochitl && ./vnsee; systemctl start xochitl
```
