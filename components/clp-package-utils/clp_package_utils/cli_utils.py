"""Utility classes for CLI argument parsing."""

import click


class RestartPolicyParamType(click.ParamType):
    """Click parameter type for Docker restart policy."""

    # The parameter type name displayed in Click help messages.
    name = "restart-policy"

    # Valid restart policies.
    VALID_POLICIES = frozenset({"no", "always", "unless-stopped", "on-failure"})

    # Human-readable string listing all valid policies for help text and error messages.
    VALID_POLICIES_STR = f"{', '.join(sorted(VALID_POLICIES))}, or on-failure:<max-retries>"

    def convert(self, value: str, param: click.Parameter | None, ctx: click.Context | None) -> str:
        """
        Validates and returns a restart policy string.

        :param value:
        :param param:
        :param ctx:
        :return: A valid Docker Compose restart policy string.
        :raise click.BadParameter: If the value is invalid.
        """
        if value in self.VALID_POLICIES:
            return value

        # Parse "on-failure:<max-retries>".
        if value.startswith("on-failure:"):
            count_str = value[len("on-failure:") :]
            if count_str.isdigit() and int(count_str) > 0:
                return value
            self.fail("max-retries must be a positive integer.", param, ctx)

        self.fail(f"Must be one of: {self.VALID_POLICIES_STR}.", param, ctx)


RESTART_POLICY = RestartPolicyParamType()
