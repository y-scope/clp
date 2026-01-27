"""Exception types used by the yscope_clp_core Python package."""


class ClpCoreError(Exception):
    """Base exception class for all yscope_clp_core exceptions."""


class ClpCoreRuntimeError(ClpCoreError, RuntimeError):
    """Generic yscope_clp_core RuntimeError."""


class ClpCoreTypeError(ClpCoreError, TypeError):
    """Generic yscope_clp_core TypeError."""


class ClpCoreValueError(ClpCoreError, ValueError):
    """Generic yscope_clp_core ValueError."""
