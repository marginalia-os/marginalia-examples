# Native Storage Service

This is the first small native package fixture for the ABI-minor-2 package
storage boundary. Its Service reads `data/state.bin`; when the file is not
present, it writes a short version marker through the typed asynchronous data
API.

The source is deliberately freestanding. It does not include Arduino,
FreeRTOS, a filesystem header, a raw SD path, or a Marginalia firmware header.
The only persistent capability it receives is the host-owned
`package.storage` contract from the SDK ABI header.

## Status

This example is an implementation and archive/preflight fixture, not a claim
that every installed native component is runnable on every firmware build.
The ESP32-C3 artifact is built with the PlatformIO RISC-V toolchain and is
still hardware-gated until the loader, Service host, package-store recovery,
and X3/X4 behavior are exercised together.

The user may install or keep the package when the host is unavailable. In that
case the firmware must report the capability result and not start the Service;
it must not silently delete the package or its data.

## Build

From this directory:

```sh
./build_native.sh
```

The script updates the inner artifact hash in `manifest.json` and emits a
deterministic native ELF at `bin/esp32-c3/module.native`. Build the archive
from the examples repository with the SDK builder after that step.

The artifact uses only the loader-supported RISC-V `RELATIVE` relocation form.
It must pass the SDK preflight and the firmware verifier before it is eligible
for executable admission.
