
import { Table} from 'antd';
import { DashboardCard } from '../../../components/DashboardCard';
import styles from "./index.module.css";
import { jobColumns, JobData} from './typings';

// eslint-disable-next-line no-warning-comments
// TODO: Replace with values from database once api implemented.
const DUMMY_DATA: JobData[] = [
  {
    key: '1',
    status: 'success',
    jobId: '1',
    speed: '66 B/s',
    dataIngested: '267 B',
    compressedSize: '460 B',
  },
  {
    key: '3',
    status: 'success',
    jobId: '3',
    speed: '10 KB/s',
    dataIngested: '50 KB',
    compressedSize: '5 KB',
  },
  {
    key: '5',
    status: 'success',
    jobId: '5',
    speed: '500 B/s',
    dataIngested: '1 KB',
    compressedSize: '800 B',
  },
  {
    key: '2',
    status: 'processing',
    jobId: '2',
    speed: '5 KB/s',
    dataIngested: '17 KB',
    compressedSize: '1 KB',
  },
  {
    key: '4',
    status: 'processing',
    jobId: '4',
    speed: '1 MB/s',
    dataIngested: '10 MB',
    compressedSize: '8 MB',
  },
  {
    key: '6',
    status: 'error',
    jobId: '6',
    speed: '0 B/s',
    dataIngested: '0 B',
    compressedSize: '0 B',
  },
  {
    key: '7',
    status: 'warning',
    jobId: '7',
    speed: '100 B/s',
    dataIngested: '500 B',
    compressedSize: '450 B',
  },
];


/**
 * Renders table with ingestion jobs inside a DashboardCard.
 *
 * @return
 */
const Jobs = () => {
  return (
    <DashboardCard title="Ingestion Jobs">
      <Table<JobData>
        className={styles["jobs"] || ""}
        columns={jobColumns}
        dataSource={DUMMY_DATA}
        pagination={false}
      />
    </DashboardCard>
  );
};

export default Jobs;
