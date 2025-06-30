import {QueryClient} from "@tanstack/react-query";


const DEFAULT_STALE_TIME_MILLIS = 10_000;

const queryClient = new QueryClient({
    defaultOptions: {
        queries: {
            staleTime: DEFAULT_STALE_TIME_MILLIS,
        },
    },
});

export default queryClient;
