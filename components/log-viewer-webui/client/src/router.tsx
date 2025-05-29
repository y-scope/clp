import {createBrowserRouter} from "react-router";

import MainLayout from "./components/Layout/MainLayout";
import IngestPage from "./pages/IngestPage";
import SearchPage from "./pages/SearchPage";
import QueryStatus from "./ui/QueryStatus";

const router = createBrowserRouter([
    {
        path: "/",
        Component: MainLayout,
        children: [
            {path: "ingest", Component: IngestPage},
            {path: "search", Component: SearchPage},
        ],
    },
    {
    path: "/stream",
        Component: QueryStatus,
    },
]);


export default router;
