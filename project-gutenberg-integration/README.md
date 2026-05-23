# Project Gutenberg Integration

Reference `integration` package that adds Project Gutenberg as an OPDS catalog provider.

Current behavior:

- installs like any other Marginalia package
- appears in the OPDS browser picker when enabled
- opens the Project Gutenberg OPDS catalog
- supports search through Gutenberg's OPDS query endpoint
- downloads EPUB books through the existing OPDS browser flow

This package does not run arbitrary code on the reader. Firmware reads the static `src/opds.json` descriptor and exposes
the provider as a read-only catalog preset.
