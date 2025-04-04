import {createBrowserRouter} from "react-router";

import IngestView from "../ui/IngestView";
import MainLayout from "../ui/MainLayout";
import SearchView from "../ui/SearchView";


const router = createBrowserRouter([
    {
        path: "/",
        Component: MainLayout,
        children: [
            {path: "ingest", Component: IngestView},
            {path: "search", Component: SearchView},
        ],
    },
]);

export default router;
