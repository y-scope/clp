import logging

import mariadb
import mysql.connector
from mysql.connector import errorcode

from clp_py_utils.clp_config import Database


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
