"""Exception types used by the clp_core Python package."""


class ClpCoreError(Exception):
    """Base exception for clp_core."""


class ClpCoreRuntimeError(ClpCoreError, RuntimeError):
    """Generic RuntimeError exception for clp_core."""


class BadCompressionInputError(ClpCoreError, ValueError):
    """Raised when compression input is invalid or unsupported."""
