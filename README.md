# Marginalia Examples

Official reference packages for Marginalia.

This repo shows the expected layout for packages that target the Marginalia SDK and registry.

## Included examples

- `dark-mode-theme`
- `game-of-life-sleep-screen`
- `reading-stats-module`
- `hangman-app`
- `project-gutenberg-integration`
- `declarative-theme-v2`
- `native-storage-service`

Hardware-only native fixtures are kept under `hardware-fixtures/`; they are
not part of the publishable example sweep. The first one is
`hardware-fixtures/native-smoke-app`, a bounded foreground App fixture for the
X3/X4 loader and recovery matrix.

Each example is a complete side-loadable folder for current firmware package management. The firmware can upload package
folders or SDK-built `.mpkg.zip` archives, stage them from the inbox, enable or disable them, and uninstall them.
Installation is not a hot swap: firmware may return `activationPending`, then promotes the candidate on the next boot
and keeps the previous version until the trial is confirmed. An example README must distinguish staged installation from
runtime support and must not claim that a package is running merely because its archive was accepted.
Uninstall is also staged: firmware may return `removalPending` until reboot and a short health trial confirms removal;
the running process is not hot-removed and package state is preserved by default.
Package and component enablement is a separate user-controlled state transition. On a running device, firmware persists
the requested state and crosses a runtime barrier that releases foreground Apps, Services, Providers, and package-data
work before rebuilding one package snapshot. A `202` response means the state is saved but cleanup/reload is still
pending; firmware retains the old executable snapshot and retries. Enabling a package never installs or enables its
dependencies automatically.
Runtime execution in the legacy examples is intentionally still represented by placeholder `src/entrypoints.json` files.
The firmware now has the first native Service host and the SDK/firmware ABI-minor-2 package asset/data storage records.
`native-storage-service` is the first real ESP32-C3 native artifact fixture: it is preflighted and admitted by the
Service compatibility profile, but remains hardware-gated until loader, recovery, and X3/X4 behavior are exercised
together. Native App and Provider examples remain gated on the foreground frame/router and typed provider ABI
respectively. Static contribution packages, including the manifest v2 `declarative-theme-v2` fixture and Project
Gutenberg, do not need runtime entrypoints.

The hardware fixture is deliberately a separate category: its ESP32-C3 ELF
can be built and preflighted, but the SDK and firmware still keep App role
admission gated until physical X3/X4 results exist.

Manifest v2 examples should keep `dataSchema` stable across releases until a firmware migration contract exists. The
package store preserves user state outside the archive; adding a component or changing its availability is not a reason
to reset settings automatically. Component IDs are stable state identities: changing a component's role or contract
requires a new component ID instead of silently reusing the old state.

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

## Build archives

```sh
mkdir -p dist
for manifest in */manifest.json; do
  package_dir="${manifest%/manifest.json}"
  python3 ../marginalia-sdk/tools/build_package.py "$package_dir" --profile publish --output dist
done
```

Generated archives are release artifacts and are ignored in this repo.

## Published archives

Release assets are published as `.mpkg.zip` files. The Dark Mode package is available at:

```text
https://github.com/marginalia-os/marginalia-examples/releases/download/examples-v0.1.6/org.marginalia.examples.dark-mode-0.1.6.mpkg.zip
```
