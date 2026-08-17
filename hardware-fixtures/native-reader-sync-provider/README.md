# Native `reader.sync/1` Provider

This is a deterministic, hardware-only Provider fixture for the Xteink X3/X4
native-runtime matrix. It implements the public `reader.sync/1` ABI shape but
does not open a socket, read credentials, access package storage, create an
RTOS object, or touch raw hardware.

The scripted behavior is deliberately small:

- `FetchRemote`: pending, one fixed remote-progress event, then completed;
- `UploadLocal`: pending, then completed;
- `Cancel`: one cancelled terminal event.

The firmware owns job generations, polling, wake times, cancellation, resource
accounting, and recovery attribution. The fixture only retains its host table
and a small state record, and all response records use bounded ABI arrays.

This package is not a product sync implementation and is not a registry or Hub
release. A valid ELF, SDK archive, or installed package proves that the public
package shape can be inspected; it does not prove that the Provider is admitted
or runnable. Current firmware keeps native Provider admission closed until the
network/credential capability host and X3/X4 recovery matrix are complete.

## Build

```sh
./build_native.sh
python3 ../../../marginalia-sdk/tools/build_package.py . \
  --profile local --output /tmp/marginalia-hardware-fixtures
```

The build script updates the inner artifact digest and runs the SDK ELF
preflight. The normal SDK compatibility validator intentionally reports
`unsupported_native_abi` for this package because the current
X3/X4 profile keeps Provider admission closed; `build_package.py
--profile local` still validates the schema, artifact binding, and archive
shape. Firmware host tests cover malformed output, stale generations,
cancellation, terminal cleanup, and unavailable capability branches; those
tests are not a substitute for executing this RISC-V image on both boards.

## RT-Thread comparison

RT-Thread modules can often create their own threads and call exported kernel or
device services. This fixture uses the same modular distribution idea but keeps
ownership with Marginalia: `ProviderHost` invokes bounded callbacks and the
typed `reader.sync/1` records are the only integration surface. That extra
boundary is required because X3/X4 do not provide an MMU process boundary for a
late-installed package.
