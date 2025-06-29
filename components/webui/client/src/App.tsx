import {RouterProvider} from "react-router";

import {
    QueryClient,
    QueryClientProvider,
} from "@tanstack/react-query";
import {ReactQueryDevtools} from "@tanstack/react-query-devtools";
import {ConfigProvider} from "antd";

import router from "./router";
import THEME_CONFIG from "./theme";

import "@ant-design/v5-patch-for-react-19";


const queryClient = new QueryClient();

/**
 * Renders Web UI app.
 *
 * @return
 */
const App = () => {
    return (
        <QueryClientProvider client={queryClient}>
            <ConfigProvider
                theme={THEME_CONFIG}
            >
                <RouterProvider router={router}/>
            </ConfigProvider>
            <ReactQueryDevtools initialIsOpen={false}/>
        </QueryClientProvider>
    );
};

export default App;
