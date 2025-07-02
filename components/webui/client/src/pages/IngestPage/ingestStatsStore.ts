import {create} from "zustand";


interface IngestStatsValues {
    refreshInterval: number;
}

interface IngestStatsActions {
    setRefreshInterval: (newRefreshInterval: number)=> void;
}

/**
 * Default values for the ingest stats store.
 */
const INGEST_STATS_DEFAULT: IngestStatsValues = Object.freeze({
    refreshInterval: 10_000,
});

type IngestStatsState = IngestStatsValues & IngestStatsActions;

const useIngestStatsStore = create<IngestStatsState>((set) => ({
    ...INGEST_STATS_DEFAULT,
    setRefreshInterval: (newRefreshInterval: number) => {
        set(() => ({refreshInterval: newRefreshInterval}));
    },
}));


export default useIngestStatsStore;
