"""Exception types used by the yscope_clp_core Python package."""


class ClpCoreError(Exception):
    """Base exception for yscope_clp_core."""


class ClpCoreRuntimeError(ClpCoreError, RuntimeError):
    """Generic RuntimeError exception for yscope_clp_core."""


class BadArchiveSourceError(ClpCoreError, TypeError):
    """Raised when the archive source has an invalid type."""


class BadCompressionInputError(ClpCoreError, TypeError):
    """Raised when compression input has an invalid type."""
