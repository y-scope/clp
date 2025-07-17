import {QueryClient} from "@tanstack/react-query";


const DEFAULT_REFETCH_INTERVAL_MILLIS = 10_000;

const queryClient = new QueryClient({
    defaultOptions: {
        queries: {
            refetchInterval: DEFAULT_REFETCH_INTERVAL_MILLIS,
        },
    },
});

export default queryClient;
