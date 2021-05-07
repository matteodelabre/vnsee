# Changelog

## v0.4.0

- Add reMarkable 2 support (depends on [remarkable2-framebuffer](https://github.com/ddvk/remarkable2-framebuffer)).
- Tolerate server resolutions differing from 1404x1872 or 1408x1872.
    - The server image is cropped to fit in the screen.
- Add `vnsee-gui` script to start VNSee from a GUI.
    - This is a [simple](https://rmkit.dev/apps/sas) script.

## v0.3.1

- Fix compatibility with TigerVNC.
    - Fill-in the `depth` PixelFormat flag appropriately.

## v0.3.0

- Rename from rmvncclient to VNSee.
- Update libvncserver to 0.9.13.
- Add `--help` flag to show a help message.
- Add `--version` flag to show the current version number.
- Add `--no-buttons`, `--no-pen` and `--no-touch` flags to disable interactions with the respective input devices.
- Improve repaint latency (thanks @asmanur!)
    - Periodically trigger repaints during long update batches instead of waiting for the end of each batch.
    - Repaint continuously in DU mode when using the pen.

## v0.2.0

- Add support for pen input.
    - Do not support pressure and tilt sensitivity yet.
- Allow refreshing the screen manually by using the Home button.
- Allow quitting the app by using the Power button.

## v0.1.0

- Initial release.
