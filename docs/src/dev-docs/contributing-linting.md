# Linting

Before submitting a PR, ensure you've run our linting tools and either fixed any violations or
suppressed the warning. If you can't run the linting workflows locally, you can enable and run the
[clp-lint] and [clp-artifact-build] workflows in your fork.

## Requirements

We currently support running our linting tools on Linux and macOS. If you're developing on another
OS, you can submit a [feature request][feature-req], or use our [clp-lint] workflow in your fork.

To run the linting tools, besides commonly installed tools like `tar`, you'll need:

* `curl`
* `md5sum`
* Python >= 3.10
* python3-venv (for the version of Python installed)
* [Task] >= 3.48.0
* [uv] >= 0.8

## Running the linters

To perform the linting checks:

```shell
task lint:check
```

To also apply any automatic fixes:

```shell
task lint:fix
```

[clp-artifact-build]: https://github.com/y-scope/clp/blob/DOCS_VAR_CLP_GIT_REF/.github/workflows/clp-artifact-build.yaml
[clp-lint]: https://github.com/y-scope/clp/blob/DOCS_VAR_CLP_GIT_REF/.github/workflows/clp-lint.yaml
[clp-issue-872]: https://github.com/y-scope/clp/issues/872
[feature-req]: https://github.com/y-scope/clp/issues/new?assignees=&labels=enhancement&projects=&template=feature-request.yaml
[Task]: https://taskfile.dev/
[uv]: https://docs.astral.sh/uv/
