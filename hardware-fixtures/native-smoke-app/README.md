# Native smoke App

This is the minimal foreground App fixture for the X3/X4 modular-runtime
hardware matrix. It draws a deterministic 1-bit pattern into the complete
OS-owned framebuffer, accepts semantic input events, allocates one small
host-owned state object, and requests exit when the logical power button is
pressed.

The fixture is intentionally not a game or a user-facing example. It exists
to answer the hardware questions that host tests cannot answer:

- can the ESP32-C3 loader materialize and execute the admitted image;
- does the foreground loop stay within its callback and frame budgets;
- does repeated enter/exit reclaim executable memory and package heap;
- does a failed callback leave the shell and recovery journal usable?

The package remains outside the production App compatibility profile until
the X3/X4 matrix has a passing result. A successful archive build or ELF
preflight is not a runtime or safety verdict.

## Build

```sh
./build_native.sh
```

The script updates the inner artifact digest and runs the SDK ELF preflight.
