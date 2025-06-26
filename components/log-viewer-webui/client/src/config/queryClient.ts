import {QueryClient} from "@tanstack/react-query";


const queryClient = new QueryClient({
    defaultOptions: {
        queries: {
            staleTime: 10_000,
        },
    },
});

export default queryClient;
