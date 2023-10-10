import sys
from abc import ABC, abstractmethod
from datetime import datetime, timedelta
from logging import Logger
from typing import Any, Dict, List, Optional, Tuple

from bson import ObjectId
from pymongo import MongoClient, ASCENDING
from pymongo.database import Database
from pymongo.collection import Collection

from .common import JobStatus  # type: ignore


class DBManager(ABC):
    @abstractmethod
    def get_db_config(self) -> Dict[str, Any]:
        """
        :return: A kwargs dictionary that can be used to initiate a new database
        manager object that connects to the same database.
        """
        pass


class MongoDBManager(DBManager):
    def __init__(self, logger: Optional[Logger], db_uri: str) -> None:
        self.__logger: Optional[Logger] = logger
        self.__db_config: Dict[str, Any] = {"logger": None, "db_uri": db_uri}
        self.__db_client: MongoClient = MongoClient(db_uri)
        self.__archive_db: Database = self.__db_client.get_default_database()
        self.__db_collections: Dict[str, Collection] = {
            "search_jobs_metrics": self.__archive_db["search_jobs_metrics"],
            "search_tasks_metrics": self.__archive_db["search_tasks_metrics"]
        }

        self.__db_collections["search_jobs_metrics"].create_index("creation_time")
        self.__db_collections["search_tasks_metrics"].create_index("job_id")

    @staticmethod
    def prepare_job_document(job_id, creation_ts, detection_ts, enqueue_ts, status):
        job_document = {
            "job_id": job_id,
            "creation_ts": creation_ts,
            "enqueue_ts": enqueue_ts,
            "schedule_time": detection_ts - creation_ts,
            "enqueue_time": enqueue_ts - detection_ts,
            "status": status
        }
        return job_document

    def get_db_config(self) -> Dict[str, Any]:
        return self.__db_config

    def insert_jobs(self, jobs: List[Dict[str, Any]]) -> None:
        self.__db_collections["search_jobs_metrics"].insert_many(jobs)

    def insert_tasks(self, jobs: List[Dict[str, Any]]):
        self.__db_collections["search_tasks_metrics"].insert_many(jobs)

    def update_job_status(self, job_id: str, status: str):
        filter_criteria = {"job_id": job_id}
        update_operation = {"$set": {"status": status}}
        self.__db_collections["search_jobs_metrics"].find_one_and_update(filter_criteria, update_operation)

    def finalize_job_stats(self, job_id: str, completion_ts: float, stats: Dict[str, Any]) -> None:
        job_filter = {"job_id": job_id}

        document = self.__db_collections["search_jobs_metrics"].find_one(job_filter)
        if document:
            # Access the value of the specific field
            creation_ts = document.get("creation_ts")
            if creation_ts is None:
                self.__logger.error(f"creation_ts not found in the document {document}")
            enqueue_ts = document.get("enqueue_ts")
            if enqueue_ts is None:
                self.__logger.error(f"enqueue_ts not found in the document {document}")
        else:
            self.__logger.error(f"job id {job_id} not found.")

        stats["end_to_end_time"] = completion_ts - creation_ts
        stats["worker_time"] = completion_ts - enqueue_ts

        update_operation = {"$set": stats}

        self.__db_collections["search_jobs_metrics"].find_one_and_update(job_filter, update_operation)