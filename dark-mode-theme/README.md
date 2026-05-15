# Dark Mode Theme

Reference `theme` module package.

When installed and enabled on supported firmware, this package enables an OS-wide inverted screen theme. The package
uses `src/theme.json` to declare a firmware-hosted theme mode, so it remains a normal `.mpkg.zip` package rather than a
hard-coded firmware setting.

Current behavior:

- inverts normal OS display updates after each screen render
- exposes an extension setting named `Invert screen`
- leaves the package install, enable, disable, and uninstall flow unchanged
- unloads cleanly when the package is disabled or uninstalled

Release archive:

```text
https://github.com/marginalia-os/marginalia-examples/releases/download/examples-v0.1.2/org.marginalia.examples.dark-mode-0.1.2.mpkg.zip
```
