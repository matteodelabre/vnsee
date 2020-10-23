# Changelog

## Next version (unpublished)

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
