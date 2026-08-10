"""Tests for compression scheduler archive storage metrics."""

# ruff: noqa: D102, PT009, SLF001

import threading
import unittest
from decimal import Decimal
from unittest.mock import MagicMock, patch

from clp_py_utils.clp_config import StorageEngine

from job_orchestration.scheduler.compress import compression_scheduler


class ArchiveStorageMetricsPollerTest(unittest.TestCase):
    """Tests archive storage aggregation, caching, and polling."""

    @staticmethod
    def _valid_row(
        compressed_bytes: object = 100,
        uncompressed_bytes: object = 1_000,
        minimum_compressed_size: object = 100,
        minimum_uncompressed_size: object = 1_000,
    ) -> dict[str, object]:
        return {
            "bytes_compressed": compressed_bytes,
            "bytes_uncompressed": uncompressed_bytes,
            "minimum_compressed_size": minimum_compressed_size,
            "minimum_uncompressed_size": minimum_uncompressed_size,
        }

    @staticmethod
    def _create_adapter(
        rows: list[dict[str, object] | BaseException],
    ) -> tuple[MagicMock, MagicMock, MagicMock]:
        cursor = MagicMock()
        cursor.fetchone.side_effect = rows
        connection = MagicMock()
        connection.cursor.return_value = cursor
        adapter = MagicMock()
        adapter.create_connection.return_value = connection
        return adapter, connection, cursor

    @staticmethod
    def _create_poller(
        adapter: MagicMock,
        storage_engine: StorageEngine = StorageEngine.CLP,
        polling_interval_secs: float = 60,
    ) -> compression_scheduler._ArchiveStorageMetricsPoller:
        return compression_scheduler._ArchiveStorageMetricsPoller(
            adapter,
            storage_engine,
            "clp_",
            polling_interval_secs,
        )

    def setUp(self) -> None:
        self.original_poller = compression_scheduler._archive_storage_metrics_state.poller

    def tearDown(self) -> None:
        compression_scheduler._archive_storage_metrics_state.poller = self.original_poller

    def test_collects_clp_root_archive_table(self) -> None:
        adapter, connection, cursor = self._create_adapter(
            [self._valid_row(Decimal(100), Decimal(1000))]
        )
        poller = self._create_poller(adapter)

        self.assertEqual((100, 1_000), poller._collect_snapshot())

        adapter.create_connection.assert_called_once_with(True)
        connection.cursor.assert_called_once_with(dictionary=True)
        cursor.close.assert_called_once_with()
        connection.close.assert_called_once_with()
        query = cursor.execute.call_args.args[0]
        self.assertIn("FROM `clp_archives`", query)
        self.assertIn("SUM(`size`)", query)
        self.assertIn("SUM(`uncompressed_size`)", query)

    def test_empty_clp_archive_table_produces_zeroes(self) -> None:
        adapter, _, _ = self._create_adapter([self._valid_row(0, 0, 0, 0)])
        poller = self._create_poller(adapter)

        self.assertEqual((0, 0), poller._collect_snapshot())

    def test_clp_s_without_datasets_produces_zeroes(self) -> None:
        adapter, _, cursor = self._create_adapter([])
        poller = self._create_poller(adapter, StorageEngine.CLP_S)

        with patch.object(compression_scheduler, "fetch_existing_datasets", return_value=set()):
            self.assertEqual((0, 0), poller._collect_snapshot())

        cursor.execute.assert_not_called()

    def test_collects_all_clp_s_dataset_archive_tables(self) -> None:
        adapter, _, cursor = self._create_adapter(
            [
                self._valid_row(100, 1_000),
                self._valid_row(200, 2_000),
            ]
        )
        poller = self._create_poller(adapter, StorageEngine.CLP_S)

        with patch.object(
            compression_scheduler,
            "fetch_existing_datasets",
            return_value={"dataset_b", "dataset_a"},
        ) as fetch_datasets:
            self.assertEqual((300, 3_000), poller._collect_snapshot())

        fetch_datasets.assert_called_once_with(cursor, "clp_")
        queries = [call.args[0] for call in cursor.execute.call_args_list]
        all_queries = "".join(queries)
        self.assertIn("FROM `clp_dataset_a_archives`", all_queries)
        self.assertIn("FROM `clp_dataset_b_archives`", all_queries)
        self.assertTrue(all("FROM `clp_archives`" not in query for query in queries))

    def test_each_poll_uses_a_fresh_connection(self) -> None:
        first_adapter, first_connection, first_cursor = self._create_adapter([self._valid_row()])
        _, second_connection, second_cursor = self._create_adapter([self._valid_row()])
        first_adapter.create_connection.side_effect = [first_connection, second_connection]
        poller = self._create_poller(first_adapter)

        poller._collect_snapshot()
        poller._collect_snapshot()

        self.assertEqual(2, first_adapter.create_connection.call_count)
        first_cursor.close.assert_called_once_with()
        first_connection.close.assert_called_once_with()
        second_cursor.close.assert_called_once_with()
        second_connection.close.assert_called_once_with()

    def test_partial_clp_s_failure_invalidates_previous_snapshot(self) -> None:
        adapter, _, _ = self._create_adapter(
            [self._valid_row(), RuntimeError("dataset table disappeared")]
        )
        poller = self._create_poller(adapter, StorageEngine.CLP_S)
        poller._snapshot = (10, 100)

        with (
            patch.object(
                compression_scheduler,
                "fetch_existing_datasets",
                return_value={"dataset_a", "dataset_b"},
            ),
            patch.object(compression_scheduler.logger, "exception") as log_exception,
        ):
            poller._poll_once()

        self.assertIsNone(poller.get_snapshot())
        log_exception.assert_called_once_with("Failed to collect archive storage metrics.")

    def test_invalid_values_invalidate_entire_snapshot(self) -> None:
        invalid_values = [
            ("bytes_compressed", None),
            ("bytes_compressed", True),
            ("bytes_compressed", "1"),
            ("bytes_compressed", 1.0),
            ("bytes_compressed", Decimal("1.5")),
            ("bytes_compressed", Decimal("NaN")),
            ("bytes_compressed", -1),
        ]

        for field_name, invalid_value in invalid_values:
            with self.subTest(field_name=field_name, invalid_value=invalid_value):
                row = self._valid_row()
                row[field_name] = invalid_value
                adapter, _, _ = self._create_adapter([row])
                poller = self._create_poller(adapter)
                poller._snapshot = (10, 100)

                with patch.object(compression_scheduler.logger, "exception"):
                    poller._poll_once()

                self.assertIsNone(poller.get_snapshot())

    def test_successful_poll_recovers_after_failure(self) -> None:
        poller = self._create_poller(MagicMock())

        with patch.object(
            poller,
            "_collect_snapshot",
            side_effect=[RuntimeError("database unavailable"), (20, 200)],
        ):
            with patch.object(compression_scheduler.logger, "exception"):
                poller._poll_once()
            self.assertIsNone(poller.get_snapshot())

            poller._poll_once()
            self.assertEqual((20, 200), poller.get_snapshot())

    def test_callbacks_only_read_cached_snapshot(self) -> None:
        adapter = MagicMock()
        poller = self._create_poller(adapter)
        poller._snapshot = (20, 200)
        compression_scheduler._archive_storage_metrics_state.poller = poller

        compressed_observations = list(
            compression_scheduler._observe_archive_bytes_compressed(MagicMock())
        )
        uncompressed_observations = list(
            compression_scheduler._observe_archive_bytes_uncompressed(MagicMock())
        )

        self.assertEqual(20, compressed_observations[0].value)
        self.assertEqual(200, uncompressed_observations[0].value)
        adapter.create_connection.assert_not_called()

    def test_callbacks_omit_values_without_valid_snapshot(self) -> None:
        poller = self._create_poller(MagicMock())
        compression_scheduler._archive_storage_metrics_state.poller = poller

        self.assertEqual(
            [], list(compression_scheduler._observe_archive_bytes_compressed(MagicMock()))
        )
        self.assertEqual(
            [], list(compression_scheduler._observe_archive_bytes_uncompressed(MagicMock()))
        )

    def test_run_polls_before_waiting_and_uses_configured_interval(self) -> None:
        poller = self._create_poller(MagicMock(), polling_interval_secs=12.5)
        poller._stop_event = MagicMock()
        poller._stop_event.is_set.return_value = False
        poller._stop_event.wait.return_value = True

        with patch.object(poller, "_poll_once") as poll_once:
            poller._run()

        poll_once.assert_called_once_with()
        poller._stop_event.wait.assert_called_once_with(12.5)

    def test_start_polls_immediately_and_stop_interrupts_wait(self) -> None:
        poller = self._create_poller(MagicMock(), polling_interval_secs=60)
        first_poll_completed = threading.Event()

        with patch.object(poller, "_poll_once", side_effect=first_poll_completed.set):
            poller.start()
            self.assertTrue(first_poll_completed.wait(timeout=1))
            poller.stop()

        self.assertFalse(poller._thread.is_alive())
        self.assertIsNone(poller.get_snapshot())

    def test_stop_uses_bounded_wait_when_database_poll_is_stuck(self) -> None:
        poller = self._create_poller(MagicMock())
        poller._thread = MagicMock()
        poller._thread.is_alive.return_value = True

        with patch.object(compression_scheduler.logger, "warning") as log_warning:
            poller.stop()

        self.assertTrue(poller._stop_event.is_set())
        poller._thread.join.assert_called_once_with(
            timeout=compression_scheduler._ARCHIVE_STORAGE_METRICS_SHUTDOWN_TIMEOUT_SECS
        )
        log_warning.assert_called_once_with(
            "Archive storage metrics poller did not stop before shutdown."
        )
        self.assertIsNone(poller.get_snapshot())

    def test_completed_poll_does_not_publish_after_stop(self) -> None:
        poller = self._create_poller(MagicMock())
        poller._stop_event.set()

        with patch.object(poller, "_collect_snapshot", return_value=(20, 200)):
            poller._poll_once()

        self.assertIsNone(poller.get_snapshot())


class ArchiveStorageMetricsStartupTest(unittest.TestCase):
    """Tests poller startup behavior."""

    def test_skips_poller_when_telemetry_is_disabled(self) -> None:
        with (
            patch.object(compression_scheduler, "is_telemetry_disabled_by_env", return_value=True),
            patch.object(compression_scheduler, "_ArchiveStorageMetricsPoller") as poller_class,
        ):
            poller = compression_scheduler._start_archive_storage_metrics_poller(
                MagicMock(), StorageEngine.CLP, "clp_", 60_000
            )

        self.assertIsNone(poller)
        poller_class.assert_not_called()

    def test_starts_poller_using_millisecond_interval(self) -> None:
        poller = MagicMock()
        with (
            patch.object(compression_scheduler, "is_telemetry_disabled_by_env", return_value=False),
            patch.object(
                compression_scheduler,
                "_ArchiveStorageMetricsPoller",
                return_value=poller,
            ) as poller_class,
        ):
            result = compression_scheduler._start_archive_storage_metrics_poller(
                MagicMock(), StorageEngine.CLP_S, "clp_", 60_000
            )

        self.assertIs(poller, result)
        poller_class.assert_called_once()
        self.assertEqual(StorageEngine.CLP_S, poller_class.call_args.args[1])
        self.assertEqual("clp_", poller_class.call_args.args[2])
        self.assertEqual(60, poller_class.call_args.args[3])
        poller.start.assert_called_once_with()


if __name__ == "__main__":
    unittest.main()
