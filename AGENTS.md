# AGENTS.md

Guidance for agents working in `marginalia-examples`.

## Project Role

This repo contains official reference packages for Marginalia. Examples demonstrate package layout and manifest usage for
themes, sleep screens, reader modules, integrations, and apps.

Current examples:

- `dark-mode-theme`
- `game-of-life-sleep-screen`
- `reading-stats-module`
- `hangman-app`

Runtime execution is still mostly placeholder-driven until firmware grows full module/app hosts. Do not fake runtime
support in examples that firmware cannot actually execute.

## Common Commands

Validate manifests:

```sh
python3 ../marginalia-sdk/tools/validate_manifest.py --profile publish */manifest.json
```

Build archives:

```sh
mkdir -p dist
for manifest in */manifest.json; do
  package_dir="${manifest%/manifest.json}"
  python3 ../marginalia-sdk/tools/build_package.py "$package_dir" --profile publish --output dist
done
```

## Guidelines

- Keep examples compatible with current firmware package API level.
- Use SDK schemas and tools rather than hand-waving manifest validity.
- Keep package ids stable once published.
- Update registry entries and hub catalog data when example release artifacts change.
- Generated `.mpkg.zip` archives are release artifacts and should stay out of normal source changes unless intentionally publishing.
- Example READMEs should state what works today versus expected future runtime behavior.

