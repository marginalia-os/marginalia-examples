# Marginalia Examples

Official reference packages for Marginalia.

This repo shows the expected layout for packages that target the Marginalia SDK and registry.

## Included examples

- `dark-mode-theme`
- `game-of-life-sleep-screen`
- `reading-stats-module`
- `hangman-app`

Each example is a complete side-loadable folder for current firmware package management. The firmware can upload,
install, enable, disable, and uninstall these packages now. Runtime execution is intentionally still represented by
placeholder `src/entrypoints.json` files until Marginalia grows module and app hosts.

## Try one on a device

1. Open the device web server.
2. Go to `/packages`.
3. Select one example folder with the package folder upload control.
4. Install it from the inbox.
5. Toggle or uninstall it from the package manager.

## Validate manifests

From this repo:

```sh
python3 ../marginalia-sdk/tools/validate_manifest.py --profile publish */manifest.json
```
