# Project Gutenberg Integration

Reference `integration` package that adds Project Gutenberg as a catalog provider.

Current behavior:

- installs like any other Marginalia package
- declares `execution: "static"` because it has no runtime module
- contributes a read-only Project Gutenberg OPDS catalog provider through `manifest.json`
- appears in the OPDS browser picker when enabled on firmware that supports `catalog.providers`
- supports search through Gutenberg's OPDS query endpoint
- downloads EPUB books through the existing OPDS browser flow

This package does not run arbitrary code on the reader. Firmware reads the static `contributes.catalog.providers`
declaration and exposes the provider as a catalog preset.

Expected future runtime behavior:

- keep the package-owned provider read-only and sandboxed
- allow richer provider metadata when the catalog host supports it
- add OPDS 2.0, RWPM, or OPDS-PSE providers only after firmware or runtime support exists for those formats
- avoid background sync unless the user explicitly starts a catalog action
