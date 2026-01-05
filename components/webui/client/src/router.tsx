import {
    createBrowserRouter,
    Navigate,
} from "react-router";

import MainLayout from "./components/Layout/MainLayout";
import IngestPage from "./pages/IngestPage";
import QueryStatus from "./pages/LogViewerLoadingPage/QueryStatus";
import OperationalLogsPage from "./pages/OperationalLogsPage";
import SearchPage from "./pages/SearchPage";


const router = createBrowserRouter([
    {
        path: "/",
        Component: MainLayout,
        children: [
            {
                path: "/",
                element: <Navigate
                    replace={true}
                    to={"/ingest"}/>,
            },
            {path: "ingest", Component: IngestPage},
            {path: "search", Component: SearchPage},
            {path: "logs", Component: OperationalLogsPage},
        ],
    },
    {
        path: "/streamFile",
        Component: QueryStatus,
    },
]);


export default router;
