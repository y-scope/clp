import {create} from "zustand";


interface RefreshIntervalState {
    refreshInterval: number;
    setRefreshInterval: (newRefreshInterval: number)=> void;
}


const useRefreshIntervalStore = create<RefreshIntervalState>((set) => ({
    refreshInterval: 10_000,
    setRefreshInterval: (newRefreshInterval: number) => {
        set(() => ({refreshInterval: newRefreshInterval}));
    },
}));

export default useRefreshIntervalStore;
