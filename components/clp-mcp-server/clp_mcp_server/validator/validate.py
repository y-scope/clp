"""KQL Query Validator using ANTLR4-generated parser."""

from antlr4 import CommonTokenStream, InputStream
from antlr4.error.ErrorListener import ErrorListener

from .generated.KqlLexer import KqlLexer
from .generated.KqlParser import KqlParser


# fmt: off
class KqlErrorListener(ErrorListener):
    """Custom error listener to capture syntax errors during parsing."""

    def __init__(self) -> None:
        """Initialize the KqlErrorListener."""
        super().__init__()

    def syntaxError(self, recognizer, offendingSymbol, line, column, msg, e) -> None:  # noqa: N802, PLR0913, ANN001, ARG002, N803
        """Handle syntax errors by raising a SyntaxError with detailed message."""
        err_msg = f"line {line}:{column} {msg}"
        raise SyntaxError(err_msg)
# fmt: on


def validate_query(query: str) -> tuple[bool, str | None]:
    """
    Validates a KQL query string using the ANTLR4-generated parser.

    :param query: The KQL query string to validate.
    :return: A tuple where the first element is a boolean indicating validity,
             and the second element is an error message if invalid, or None if valid.
    """
    try:
        lexer = KqlLexer(InputStream(query))
        stream = CommonTokenStream(lexer)
        parser = KqlParser(stream)

        parser.removeErrorListeners()  # Remove default ConsoleErrorListener
        parser.addErrorListener(KqlErrorListener())

        parser.start()  # Start parsing from the 'start' rule
    except SyntaxError as e:
        return False, str(e)
    else:
        return True, None
