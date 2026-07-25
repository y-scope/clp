<!-- markdownlint-disable MD012 -->

<!--
Proposed PR title:
chore(webui): Update YScope Log Viewer.
-->

# Description

Updates the generated YScope Log Viewer workspace package from upstream commit `f9b46d26cb1ddece3b1489e0fecdb8dc2515eb1c` to `20225d20e5427810d962c85bdbed5b00ac8eff3c`.

This update includes the latest decoder fixes and the upstream Node.js 24 migration. Since the generated package now declares `@types/node` `^24.13.3`, this change also removes the workspace-level `@types/node` override and regenerates the pnpm lockfile. The resulting graph continues to resolve one `@types/node` package at version 24.13.3.

# Checklist

* [x] The PR satisfies the [contribution guidelines][yscope-contrib-guidelines].
* [x] This is a breaking change and that has been indicated in the PR title, OR this isn't a breaking change.
* [x] Necessary docs have been updated, OR no docs need to be updated.

# Validation performed

```text
$ corepack pnpm install --frozen-lockfile
Scope: all 5 workspace projects
Lockfile is up to date, resolution step is skipped
Progress: resolved 1, reused 0, downloaded 0, added 0
Packages: +1267
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Progress: resolved 1267, reused 1265, downloaded 0, added 1267, done

devDependencies:
+ turbo 2.9.16

Done in 2.1s using pnpm v11.5.2

$ find node_modules/.pnpm -mindepth 1 -maxdepth 1 -type d -name '@types+node@*' -printf '%f\n' | sort -u
@types+node@24.13.3

$ task lint:fix-js
 Tasks:    4 successful, 4 total
Cached:    4 cached, 4 total
  Time:    36ms >>> FULL TURBO

$ corepack pnpm --filter yscope-log-viewer run lint:check
$ npm-run-all --sequential --continue-on-error lint:check:*

$ task lint:fix-yaml
task: [lint:yaml] . "/home/junhao/.codex/worktrees/c892/5-clp/build/lint-venv/bin/activate"
yamllint --strict \
  --config-file "tools/yscope-dev-utils/exports/lint-configs/.yamllint.yml" \
  ".github" \
  "components/core/.clang-format" \
  "components/core/config" \
  "components/core/tools/" \
  "components/package-template/src/etc" \
  "docs" \
  "taskfile.yaml" \
  "taskfiles" \
  "tools/deployment/package" \
  "tools/deployment/presto-clp"

$ task webui
 Tasks:    4 successful, 4 total
Cached:    4 cached, 4 total
  Time:    46ms >>> FULL TURBO

✓ Lockfile at ../../build/webui/server/pnpm-lock.yaml passes supply-chain policies (1387 entries in 4.6s)

$ corepack pnpm exec turbo build --filter=yscope-log-viewer --force
yscope-log-viewer:build: ✓ built in 1.43s

 Tasks:    1 successful, 1 total
Cached:    0 cached, 1 total
  Time:    1.875s

$ corepack pnpm --filter yscope-log-viewer exec jest --coverage=false
Test Suites: 2 passed, 2 total
Tests:       34 passed, 34 total
Snapshots:   0 total
Time:        1.062 s, estimated 3 s
Ran all test suites.

$ corepack pnpm --filter @webui/server run test
TAP version 14
1..1
# Subtest: src/test/example.test.ts
    # Subtest: Tests the example routes
        ok 1 - should be equal
        ok 2 - should match pattern
        ok 3 - should be equal
        ok 4 - should match pattern
        1..4
    ok 1 - Tests the example routes # time=78.793ms

    1..1
ok 1 - src/test/example.test.ts # time=670.761ms

# { total: 4, pass: 4 }
# time=712.376ms

$ git diff --check
```

The aggregate `corepack pnpm exec turbo test --force` command runs all 34 Log Viewer assertions and all 4 server assertions successfully but exits nonzero because pnpm resolves Jest 30.4.2, whose coverage result does not satisfy the vendored package's 100% global threshold. Running the same `pnpm@11.5.2` frozen install and Log Viewer test against the original `f9b46d2` package and lockfile produces the same coverage-only failure, so it is not a regression from this update.

`corepack pnpm peers check` reports the same pre-existing React 19 peer incompatibilities for `react-element-to-jsx-string@15.0.0` before and after the change; no peer warning was added.

[yscope-contrib-guidelines]: https://docs.yscope.com/dev-guide/contrib-guides-overview.html
