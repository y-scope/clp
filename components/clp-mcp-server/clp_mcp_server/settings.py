"""Settings for CLP MCP server."""

import os

CLP_DB_SERVICE_NAME = os.environ.get("CLP_DB_SERVICE_NAME", "db")
CLP_DB_PORT = int(os.environ.get("CLP_DB_PORT", "3306"))
CLP_DB_NAME = os.environ.get("CLP_DB_NAME", "clp-db")
CLP_DB_USER = os.environ.get("CLP_DB_USER", "clp-user")
CLP_DB_PASS = os.environ.get("CLP_DB_PASS", "<no_password_set>")

CLP_RESULTS_CACHE_SERVICE_NAME = os.environ.get("CLP_RESULTS_CACHE_SERVICE_NAME", "results-cache")
CLP_RESULTS_CACHE_PORT = int(os.environ.get("CLP_RESULTS_CACHE_PORT", "27017"))
CLP_RESULTS_CACHE_CLP_DB_NAME = os.environ.get("CLP_RESULTS_CACHE_CLP_DB_NAME", "clp-query-results")
