# Game of Life Sleep Screen

Reference `sleep_screen` module package.

This package is intentionally not executable yet. It demonstrates a sleep-screen package that the firmware can side-load,
install, enable, disable, and uninstall today.

Expected future behavior:

- seed a small Conway's Game of Life grid when the device enters sleep
- render an e-ink-friendly frame
- advance the grid on sleep ticks when allowed by power policy
