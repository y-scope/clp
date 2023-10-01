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

    @abstractmethod
    def get_jobs_after_timestamp(self, since: Optional[datetime]) -> List:
        pass

    @abstractmethod
    def get_job_metadata(self, job_id: str) -> Dict:
        pass

    @abstractmethod
    def get_job_status(self, job_id: str) -> JobStatus:
        pass

    @abstractmethod
    def set_job_status(self, job_id: str, status: JobStatus, **kwargs) -> bool:
        pass

    @abstractmethod
    def get_job_submission_timestamp(self, job_id: str) -> datetime:
        pass

    @abstractmethod
    def update_job_progression(self, job_id: str) -> None:
        pass

    @abstractmethod
    def update_job_metadata(self, job_id: str, metadata: Dict[str, Any]) -> None:
        pass

    @abstractmethod
    def update_compression_stat(self) -> None:
        pass


class MongoDBManager(DBManager):
    def __init__(self, logger: Optional[Logger], db_uri: str) -> None:
        self.__logger: Optional[Logger] = logger
        self.__db_config: Dict[str, Any] = {"logger": None, "db_uri": db_uri}
        self.__db_client: MongoClient = MongoClient(db_uri)
        self.__archive_db: Database = self.__db_client.get_default_database()
        self.__db_collections: Dict[str, Collection] = {
            "archives": self.__archive_db["archives"],
            "files": self.__archive_db["files"],
            "cjobs": self.__archive_db["cjobs"],
            "stats": self.__archive_db["stats"],
            "empty_directories": self.__archive_db["empty_directories"],
        }
        # V0.5 TODO hack to achieve overwrite
        self.__stat_id = ObjectId()
        self.__projection: Dict[str, int] = {
            "_id": 1,
            "input_type": 1,
            "input_config": 1,
            "output_config": 1,
            "submission_timestamp": 1,
            "status": 1,
        }

        self.__latest_query_result: Dict[str, Any] = {}

        self.__db_collections["cjobs"].create_index("submission_timestamp")

        self.__job_data_size_aggregation_pipeline: List[Dict[str, Any]] = [
            {"$match": {"_id": None}},
            {
                "$lookup": {
                    "from": "archives",
                    "localField": "archives",
                    "foreignField": "_id",
                    "as": "archive",
                }
            },
            {"$project": {"_id": 0, "archive.uncompressed_size": 1, "archive.size": 1}},
            {"$unwind": "$archive"},
            {
                "$group": {
                    "_id": None,
                    "logs_uncompressed_size": {"$sum": "$archive.uncompressed_size"},
                    "logs_compressed_size": {"$sum": "$archive.size"},
                }
            },
        ]
        self.__data_size_aggregation_pipeline: List[Dict[str, Any]] = [
            {"$project": {"uncompressed_size": 1, "size": 1}},
            {
                "$group": {
                    "_id": None,
                    "logs_uncompressed_size": {"$sum": "$uncompressed_size"},
                    "logs_compressed_size": {"$sum": "$size"},
                }
            },
        ]
        self.__time_range_aggregation_pipeline: List[Dict[str, Any]] = [
            {"$match": {"ts_patt_msg_num": {"$not": {"$size": 0}}}},
            {"$project": {"ts_begin": 1, "ts_end": 1}},
            {
                "$group": {
                    "_id": None,
                    "ts_begin_min": {"$min": "$ts_begin"},
                    "ts_end_max": {"$max": "$ts_end"},
                }
            },
        ]
        self.__num_messages_aggregation_pipeline: List[Dict[str, Any]] = [
            {"$project": {"num_messages": 1}},
            {"$group": {"_id": None, "num_messages": {"$sum": "$num_messages"}}},
        ]

    def _datetime_to_millisecond_timestamp(self, dt: datetime) -> int:
        """
        :return: The datetime converted into number of milliseconds elapsed
        since the Unix epoch time.
        """
        return round(dt.timestamp() * 1000)

    def _millisecond_timestamp_to_datetime(self, num_ms: int) -> datetime:
        """
        :return: The millisecond timestamp since the Unix epoch time converted
        into datetime.
        """

        seconds = num_ms // 1000
        microseconds = (num_ms % 1000) * 1000
        return datetime.fromtimestamp(seconds) + timedelta(microseconds=microseconds)

    def get_db_config(self) -> Dict[str, Any]:
        return self.__db_config

    def get_job_metadata(self, job_id: str) -> Dict[str, Any]:
        """Returns the metadata for the job with the provided ID. The metadata
        should contain all the fields specified in the search projection. If the
        metadata is in the local cache, it is directly returned. Otherwise, the
        database manager initiates a query to the remote database.

        :return: Metadata for the job with the given ID.
        """

        try:
            job_metadata = self.__latest_query_result[job_id]
            return job_metadata
        except KeyError:
            jobs_collection = self.__db_collections["cjobs"]
            find_filter = {"_id": ObjectId(job_id)}

            job_metadata = jobs_collection.find_one(find_filter, self.__projection)
            if job_metadata is not None:
                self.__latest_query_result[job_id] = job_metadata
            else:
                raise ValueError(f"Cannot find metadata for job with ID `{job_id}`.")
            return job_metadata

    def get_jobs_after_timestamp(self, since: Optional[datetime]) -> List[Tuple[str, JobStatus]]:
        """
        :return: A list of of tuples consisted of job id and job status for jobs
        that are submitted after the provided timestamp.
        """
        self.__latest_query_result.clear()
        return_result = []

        jobs_collection = self.__db_collections["cjobs"]
        find_filter = {}
        if since is not None:
            find_filter["submission_timestamp"] = {
                "$gt": self._datetime_to_millisecond_timestamp(since)
            }

        for job_metadata in jobs_collection.find(
            find_filter, self.__projection, sort=[("submission_timestamp", ASCENDING)]
        ):
            job_id = str(job_metadata["_id"])
            job_status_str = job_metadata["status"]

            try:
                job_status = JobStatus.from_str(job_status_str)
            except KeyError:
                if self.__logger is not None:
                    self.__logger.error(f"Unknown status `{job_status_str}` for job `{job_id}`.")
                continue

            return_result.append((job_id, job_status))
            self.__latest_query_result[job_id] = job_metadata

        return return_result

    def get_job_status(self, job_id: str) -> JobStatus:
        """
        :return: The status of the job with the given ID.
        """

        job_metadata = self.get_job_metadata(job_id)
        return JobStatus.from_str(job_metadata["status"])

    def set_job_status(
        self, job_id: str, status: JobStatus, prev_status: Optional[JobStatus] = None, **kwargs
    ) -> bool:
        """
        Sets the status of the job in both the local cache and the remote end
        database. If the `prev_status` argument is provided, the job to be
        updated must have a matching status before the update is applied, or
        else the update will not be performed. Additional keyword arguments can
        be supplied to be applied along with the status update.

        :return: True if the operation is successful.
        """

        jobs_collection = self.__db_collections["cjobs"]
        find_filter: Dict[str, Any] = {"_id": ObjectId(job_id)}
        update_op: Dict[str, Dict[str, Any]] = {"$set": {"status": str(status)}}
        for key, value in kwargs.items():
            update_op["$set"][key] = value

        if prev_status is not None:
            find_filter["status"] = str(prev_status)
            result = jobs_collection.find_one_and_update(find_filter, update_op)
            if result is None:
                return False
        else:
            result = jobs_collection.update_one(find_filter, update_op)
            if 0 == result.matched_count:
                return False

        try:
            job_metadata = self.__latest_query_result[job_id]
            job_metadata["status"] = str(status)
            for key in self.__projection:
                if key in kwargs:
                    job_metadata[key] = kwargs[key]
        except KeyError:
            pass

        return True

    def get_job_submission_timestamp(self, job_id: str) -> datetime:
        """
        :return: The datetime object that represents the submission time of the
        job with the given ID.
        """

        job_metadata = self.get_job_metadata(job_id)
        return self._millisecond_timestamp_to_datetime(job_metadata["submission_timestamp"])

    def update_job_metadata(self, job_id, metadata: Dict[str, Any]) -> None:
        find_filter = {"_id": ObjectId(job_id)}
        jobs_collection = self.__db_collections["cjobs"]
        jobs_collection.find_one_and_update(
            filter=find_filter,
            update={
                "$set": {
                    'stats': metadata,
                }
            },
        )

    # can be done with a better ts filter. but brutal force for now
    def update_compression_stat(self) -> None:
        filter = {"status": str(JobStatus.DONE)}
        projection = {'stats': 1}
        jobs_collection = self.__db_collections["cjobs"]

        final_stat = {
            'id': 'compression_stats',
            'total_uncompressed_size': 0,
            'total_compressed_size': 0,
            'num_messages': 0,
            'num_files': 0,
            'begin_ts': sys.maxsize,
            'end_ts': 0
        }

        # V0.5 TODO Deduplicate
        for doc in jobs_collection.find(filter, projection):
            stat_data = doc['stats']
            final_stat['total_uncompressed_size'] += stat_data['total_uncompressed_size']
            final_stat['total_compressed_size'] += stat_data['total_compressed_size']
            final_stat['num_messages'] += stat_data['num_messages']
            final_stat['num_files'] += stat_data['num_files']
            final_stat['begin_ts'] = min(stat_data['begin_ts'], final_stat['begin_ts'])
            final_stat['end_ts'] = max(stat_data['end_ts'], final_stat['end_ts'])
        id_filter = {
            'id': 'compression_stats',
        }
        if final_stat['begin_ts'] == sys.maxsize:
            final_stat['begin_ts'] = 0
        self.__db_collections['stats'].update_one(id_filter, {"$set": final_stat}, upsert=True)

    def update_job_progression(self, job_id: str) -> None:
        """
        Calculate and update size of uncompressed and compressed data processed by job
        """
        job_data_uncompressed_size = 0
        job_data_compressed_size = 0

        # Set aggregation pipeline to process the current job
        self.__job_data_size_aggregation_pipeline[0]["$match"]["_id"] = ObjectId(job_id)

        jobs_collection = self.__db_collections["cjobs"]
        for doc in jobs_collection.aggregate(pipeline=self.__job_data_size_aggregation_pipeline):
            job_data_uncompressed_size = doc["logs_uncompressed_size"]
            job_data_compressed_size = doc["logs_compressed_size"]

        find_filter = {"_id": ObjectId(job_id)}
        jobs_collection.find_one_and_update(
            filter=find_filter,
            update={
                "$set": {
                    "end_timestamp": datetime.now(),
                    "logs_uncompressed_size": job_data_uncompressed_size,
                    "logs_compressed_size": job_data_compressed_size,
                }
            },
        )