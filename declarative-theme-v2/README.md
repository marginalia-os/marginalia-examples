# Declarative Dark Mode (Manifest v2)

This reference package demonstrates the manifest v2 component model without native code. It contributes the existing
firmware-hosted `theme/1` contract through `src/theme.json` and can be parsed by current Marginalia firmware without
starting a package-owned task.

It is a source fixture rather than a published registry release. Native v2 examples should wait until the measured
native loader and recovery supervisor are available.
