# Reading Stats Module

Reference `reader_module` package.

This package is intentionally not executable yet. It demonstrates a reader hook package that the firmware can side-load,
stage for a later boot, enable, disable, and uninstall today.

Expected future behavior:

- observe book open and close events
- count page turns
- write compact reading-session summaries through the ABI-minor-2 package-data storage contract once this example has a
  native component and the X3/X4 storage replay fixture is validated
