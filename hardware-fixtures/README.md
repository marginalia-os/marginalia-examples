# Hardware fixtures

These packages are open-source test fixtures for the Xteink X3/X4 modular
runtime matrix. They are not registry examples and are not publishable through
the normal `*/manifest.json` example sweep.

The fixture packages can be built and ELF-preflighted before a device is
available. That does not make native App or Provider components runnable:
firmware still owns the role-specific admission gate, and the X3/X4 matrix
must record real loader, memory, display, watchdog, and recovery results first.

The native App fixture deliberately uses only the ABI's complete framebuffer,
semantic input, host allocation, and redraw/exit requests. It has no renderer,
GPIO, filesystem, socket, FreeRTOS, or raw hardware dependency.

## Validate and build

From this repository:

```sh
./native-smoke-app/build_native.sh
python3 ../marginalia-sdk/tools/build_package.py hardware-fixtures/native-smoke-app \
  --profile local --output /tmp/marginalia-hardware-fixtures
```

The resulting package is for the hardware test matrix only. Do not publish it
or treat a successful host build as evidence that a native App is admitted on
an X3 or X4.
