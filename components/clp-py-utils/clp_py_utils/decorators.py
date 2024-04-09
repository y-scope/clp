import functools


def exception_default_value(default):
    """
    A function decorator that returns the value specified by `default` when an uncaught exception
    occurs in the decorated function. Otherwise, the decorated function's return value is returned.
    :param default: The value to return upon catching an uncaught exception.
    :return: The decorator
    """

    def parametrized_decorator(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            try:
                return func(*args, **kwargs)
            except Exception:
                return default

        return wrapper

    return parametrized_decorator
