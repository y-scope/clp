import { create } from 'zustand'
import dayjs from "dayjs";

interface SearchState {
    queryString: string;
    timeRange: [dayjs.Dayjs, dayjs.Dayjs];
    updateQueryString: (query: string) => void;
    updateTimeRange: (range: [dayjs.Dayjs, dayjs.Dayjs]) => void;
}

const useSearchStore = create<SearchState>((set) => ({
    queryString: "",
    timeRange: [dayjs().startOf("day"), dayjs().endOf("day")],
    updateQueryString: (query) => set({ queryString: query }),
    updateTimeRange: (range) => set({ timeRange: range }),
}));

export default useSearchStore;
