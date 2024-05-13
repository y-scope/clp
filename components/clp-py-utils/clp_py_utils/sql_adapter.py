import contextlib
import logging
import time

import mariadb
import mysql.connector
import sqlalchemy.pool as pool
from mysql.connector import errorcode
from sqlalchemy.dialects.mysql import mariadbconnector, mysqlconnector

from clp_py_utils.clp_config import Database


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


class SQL_Adapter:
    def __init__(self, database_config: Database):
        self.database_config = database_config

    def create_mysql_connection(
        self, disable_localhost_socket_connection: bool = False
    ) -> mysql.connector.MySQLConnection:
        try:
            connection = mysql.connector.connect(
                **self.database_config.get_mysql_connection_params(
                    disable_localhost_socket_connection
                )
            )
        except mysql.connector.Error as err:
            if err.errno == errorcode.ER_ACCESS_DENIED_ERROR:
                logging.error("Database access denied.")
            elif err.errno == errorcode.ER_BAD_DB_ERROR:
                logging.error(f'Specified database "{self.database_config.name}" does not exist.')
            else:
                logging.error(err)
            raise err
        else:
            return connection

    def create_mariadb_connection(
        self, disable_localhost_socket_connection: bool = False
    ) -> mariadb.connection:
        try:
            connection = mariadb.connect(
                **self.database_config.get_mysql_connection_params(
                    disable_localhost_socket_connection
                )
            )
        except mariadb.Error as err:
            logging.error(f"Error connecting to MariaDB: {err}")
            raise err
        else:
            return connection

    def create_connection(self, disable_localhost_socket_connection: bool = False):
        if "mysql" == self.database_config.type:
            return self.create_mysql_connection(disable_localhost_socket_connection)
        elif "mariadb" == self.database_config.type:
            return self.create_mariadb_connection(disable_localhost_socket_connection)
        else:
            raise NotImplementedError

    def create_connection_pool(
        self,
        logger: logging.Logger,
        pool_size: int,
        disable_localhost_socket_connection: bool = False,
    ):
        def create_connection():
            return self.create_connection(disable_localhost_socket_connection)

        if "mysql" == self.database_config.type:
            dialect = mysqlconnector.dialect()
        elif "mariadb" == self.database_config.type:
            dialect = mariadbconnector.dialect()
        else:
            raise NotImplementedError
        return ConnectionPoolWrapper(
            pool.QueuePool(
                create_connection,
                pool_size=pool_size,
                dialect=dialect,
                max_overflow=0,
                pre_ping=True,
            ),
            logger,
        )
