# Linting

Before submitting a PR, ensure you've run our linting tools and either fixed any violations or
suppressed the warning. To run our linting workflows locally, you'll need [Task][1]. Alternatively,
you can run the [clp-lint][2] workflow in your fork.

To perform the linting checks:

```shell
task lint:check
```

To also apply any automatic fixes:

```shell
task lint:fix
```

[1]: https://taskfile.dev/
[2]: https://github.com/y-scope/clp/blob/main/.github/workflows/clp-lint.yaml
