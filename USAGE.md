```sh
gtf 1408 1872 60
xrandr --newmode "1408x1872" 225.00 1408 1520 1672 1936 1872 1873 1876 1937 -hsync +vsync
xrandr --addmode VIRTUAL1 1408x1872
xrandr --output VIRTUAL1 --auto --right-of DP1
x11vnc -repeat -forever -clip $(xrandr | awk '/VIRTUAL1 connected/{print $3}') -nocursor
```

To flip the screen upside down:

```
x11vnc -repeat -forever -clip $(xrandr | awk '/VIRTUAL1 connected/{print $3}') -nocursor -rotate xy
```

Start client:

```
ssh "root@10.11.99.1" "./rmvncclient $(ip address show up to 10.11.99.0/29 | awk -F'[ /]+' '/inet/{print $3}')"
```
