<!-- markdownlint-disable MD012 -->

# Description

## Summary
- Moved legacy WebUI app directories into `components/webui/packages/*` and adopted a `pnpm` + Turborepo workspace layout.
- Reworked WebUI build/lint/deps task orchestration to use `pnpm` (`install`, `run`, `deploy`) with checksum-based change tracking.
- Updated docs and runtime config so local dev and packaging behavior match the workspace structure.

## Changes
- `components/webui`
  - Moved/renamed legacy app directories into `components/webui/packages/client`, `components/webui/packages/common`, and `components/webui/packages/server` (with existing source/config files preserved by move).
  - Deleted `components/webui/package-lock.json`.
  - Added/updated
    - `components/webui/pnpm-lock.yaml`
    - `components/webui/pnpm-workspace.yaml` (including `injectWorkspacePackages: true` and `verifyDepsBeforeRun: false`)
    - `components/webui/turbo.json`
  - Updated `components/webui/package.json` scripts to match the workspace flow (`turbo` commands, lint filter scope).
  - Updated `components/webui/packages/server/settings.json`:
    - `LogViewerDir` now points to `../yscope-log-viewer/dist` for package-based local serving.
  - Updated `.gitignore` to include `.turbo/` as a build artifact.
- `components/webui/packages/*/package.json` and generated `components/webui/pnpm-lock.yaml`
  - Added/adjusted lint/runtime deps for package-level tooling consistency.
- go-task Taskfile definitions (`taskfile.yaml` + `taskfiles/*.yaml`)
  - `taskfile.yaml`: refactored `webui`, `webui:dist`, and `clean-webui` flows around the new package layout and removed older per-package install-check staging.
    - Removed the dedicated `webui-node-modules` checksum/checkpoint task and now run `pnpm install --frozen-lockfile` directly during `webui:dist`/lint.
    - Kept `package` using copied built `client/settings.json` and `server/dist/settings.json` as inputs so package rebuilds are triggered by those outputs.
    - Expanded `clean-webui` to wipe package-level `node_modules` and `.turbo` directories.
  - `taskfiles/codegen.yaml`: updated parser generation paths to `packages/client`.
  - `taskfiles/deps/log-viewer.yaml`: switched to workspace source usage and checksum excludes (`.turbo`, `dist`, `node_modules`).
    - Removed previous log-viewer build/checksum tasks and now rely on downloading the vendored viewer source plus `pnpm` dependency installation.
  - `taskfiles/lint.yaml`: switched WebUI lint execution to `pnpm` and updated source globs for package `dist`/`node_modules` trees.
  - `taskfiles/toolchains.yaml`: aligned nodejs task var usage and added corepack setup for pnpm usage in WebUI.
- `docs/src/dev-docs/components-webui.md`
  - Updated WebUI local setup/run commands from root-level `npm` invocations to workspace-aware `pnpm` commands (`pnpm install`, `pnpm run dev`, and `pnpm --filter ... run dev`).

# Checklist

* [ ] The PR satisfies the [contribution guidelines][yscope-contrib-guidelines].
* [ ] This is a breaking change and that has been indicated in the PR title, OR this isn't a
  breaking change.
* [ ] Necessary docs have been updated, OR no docs need to be updated.

# Validation performed

- Manually reviewed the diff against `origin/main` for each changed path referenced above.
- No functional test suite was run in this pass; CI/task execution remains the final validation.

[yscope-contrib-guidelines]: https://docs.yscope.com/dev-guide/contrib-guides-overview.html
