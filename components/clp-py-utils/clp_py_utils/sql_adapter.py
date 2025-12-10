"""SQL adapter for connecting to a MySQL-compatible database."""

import contextlib
import logging
import time

import mariadb
import mysql.connector
from mysql.connector import errorcode
from sqlalchemy import pool
from sqlalchemy.dialects.mysql import mariadbconnector, mysqlconnector

from clp_py_utils.clp_config import ClpDbUserType, Database, DatabaseEngine


class DummyCloseableObject:
    def close(self):
        pass


class ConnectionPoolWrapper:
    """
    A wrapper around a sqlalchemy connection pool in order to simplify error handling, and prevent
    log spew when the database is down.
    """

    def __init__(self, pool: pool.QueuePool, logger: logging.Logger):
        self.pool = pool
        self.time_of_last_exception_log = None
        self.error_reporting_interval = 60  # one minute
        self.logger = logger

    def connect(self):
        """
        Checks out a connection from the pool.
        :return: On success, a connection from the pool. On error, a DummyCloseableObject.
        """
        try:
            return self.pool.connect()
        except (mariadb.Error, mysql.connector.Error):
            # Periodically log the error so long as a connection error hasn't occurred within the
            # past interval.
            current_time = time.time()
            if (
                self.time_of_last_exception_log is None
                or current_time - self.time_of_last_exception_log > self.error_reporting_interval
            ):
                self.time_of_last_exception_log = current_time
                self.logger.exception(
                    f"Failed to connect to database. Suppressing further connection error logs for "
                    f"{self.error_reporting_interval} seconds."
                )
            return DummyCloseableObject()

    def alive(self):
        with contextlib.closing(self.connect()) as db_conn:
            if isinstance(db_conn, DummyCloseableObject):
                return False
        return True


class SqlAdapter:
    """SQL adapter for connecting to a MySQL-compatible database."""

    def __init__(self, database_config: Database):
        """Initializes the SqlAdapter with the CLP database config model."""
        self.database_config = database_config

    def create_connection(
        self,
        disable_localhost_socket_connection: bool = False,
        user_type: ClpDbUserType = ClpDbUserType.CLP,
    ) -> mysql.connector.abstracts.MySQLConnectionAbstract | mariadb.Connection:
        """
        Creates a connection to the database.

        :param disable_localhost_socket_connection: If true, force TCP connections.
        :param user_type: User type whose credentials should be used to connect.
        :return: The connection.
        """
        if self.database_config.type == DatabaseEngine.MYSQL:
            return self._create_mysql_connection(disable_localhost_socket_connection, user_type)
        if self.database_config.type == DatabaseEngine.MARIADB:
            return self._create_mariadb_connection(disable_localhost_socket_connection, user_type)
        raise NotImplementedError

    def create_connection_pool(
        self,
        logger: logging.Logger,
        pool_size: int,
        disable_localhost_socket_connection: bool = False,
        user_type: ClpDbUserType = ClpDbUserType.CLP,
    ) -> ConnectionPoolWrapper:
        """
        Creates a connection pool to the database.

        :param logger: The logger to use for logging connection pool errors.
        :param pool_size: The size of the connection pool.
        :param disable_localhost_socket_connection: If true, force TCP connections.
        :param user_type: User type whose credentials should be used to connect.
        :return: The connection pool.
        """

        def _create_connection():
            return self.create_connection(disable_localhost_socket_connection, user_type)

        if self.database_config.type == DatabaseEngine.MYSQL:
            dialect = mysqlconnector.dialect()
        elif self.database_config.type == DatabaseEngine.MARIADB:
            dialect = mariadbconnector.dialect()
        else:
            raise NotImplementedError(
                f"Database type '{self.database_config.type}' is not supported."
            )
        return ConnectionPoolWrapper(
            pool.QueuePool(
                _create_connection,
                pool_size=pool_size,
                dialect=dialect,
                max_overflow=0,
                pre_ping=True,
            ),
            logger,
        )

    def _create_mysql_connection(
        self,
        disable_localhost_socket_connection: bool = False,
        user_type: ClpDbUserType = ClpDbUserType.CLP,
    ) -> mysql.connector.abstracts.MySQLConnectionAbstract:
        try:
            connection = mysql.connector.connect(
                **self.database_config.get_mysql_connection_params(
                    disable_localhost_socket_connection, user_type
                )
            )
        except mysql.connector.Error as err:
            if err.errno == errorcode.ER_ACCESS_DENIED_ERROR:
                logging.exception("Database access denied.")
            elif err.errno == errorcode.ER_BAD_DB_ERROR:
                logging.exception(
                    f'Specified database "{self.database_config.name}" does not exist.'
                )
            else:
                logging.exception(err)
            raise err
        else:
            return connection

    def _create_mariadb_connection(
        self,
        disable_localhost_socket_connection: bool = False,
        user_type: ClpDbUserType = ClpDbUserType.CLP,
    ) -> mariadb.Connection:
        try:
            connection = mariadb.connect(
                **self.database_config.get_mysql_connection_params(
                    disable_localhost_socket_connection, user_type
                )
            )
        except mariadb.Error as err:
            logging.exception(f"Error connecting to MariaDB: {err}")
            raise err
        else:
            return connection
