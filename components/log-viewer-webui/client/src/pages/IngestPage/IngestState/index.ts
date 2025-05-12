import axios, {HttpStatusCode} from "axios";
import dayjs from "dayjs";
import {create} from "zustand";

import {JobData} from "../Jobs/typings";
import {
    convertQueryJobsItemToJobData,
    QueryJobsResp,
} from "./QueryJobsResp";
import {
    getQueryJobsSql,
    getQueryStatsSql,
    QueryStatsResp,
} from "./sql";


interface IngestState {
    compressedSize: number;
    uncompressedSize: number;
    timeRange: [dayjs.Dayjs, dayjs.Dayjs];
    numMessages: number;
    numFiles: number;
    jobs: JobData[];
    updateStats: () => Promise<void>;
    updateJobs: () => Promise<void>;
}

/**
 * Default values of the ingest state.
 */
const INGEST_STATE_DEFAULT = Object.freeze<IngestState>({
    compressedSize: 0,
    jobs: [],
    numFiles: 0,
    numMessages: 0,
    timeRange: [dayjs(),
        dayjs()],
    uncompressedSize: 0,
    updateJobs: async () => {
    },
    updateStats: async () => {
    },
});

const JOB_DAYS_TO_SHOW: number = 30;

const useIngestStore = create<IngestState>((set) => ({
    ...INGEST_STATE_DEFAULT,
    updateStats: async () => {
        try {
            const response = await axios.post<QueryStatsResp>("/query/sql", {
                queryString: getQueryStatsSql(),
            });

            if (Number(HttpStatusCode.Ok) !== response.status) {
                throw new Error();
            }
            const [resp] = response.data;
            if ("undefined" === typeof resp) {
                throw new Error();
            }
            set(() => ({
                compressedSize: resp.total_compressed_size,
                numFiles: resp.num_files,
                numMessages: resp.num_messages,
                timeRange: [dayjs(resp.begin_timestamp),
                    dayjs(resp.end_timestamp)],
                uncompressedSize: resp.total_uncompressed_size,
            }));
        } catch (error: unknown) {
            console.error("An error occurred when fetching stats: ", error);
        }
    },
    updateJobs: async () => {
        try {
            const beginTimestamp = dayjs().subtract(JOB_DAYS_TO_SHOW, "days")
                .unix();
            const response = await axios.post<QueryJobsResp>("/query/sql", {
                queryString: getQueryJobsSql(beginTimestamp),
            });

            if (Number(HttpStatusCode.Ok) !== response.status) {
                throw new Error();
            }
            const resp = response.data;
            const jobs = resp
                .map((item): JobData => convertQueryJobsItemToJobData(item))
                .sort((a, b): number => Number(a.key) - Number(b.key));

            set(() => ({
                jobs: jobs,
            }));
        } catch (error: unknown) {
            console.error("An error occurred when fetching stats: ", error);
        }
    },
}));

export {INGEST_STATE_DEFAULT};
export default useIngestStore;
