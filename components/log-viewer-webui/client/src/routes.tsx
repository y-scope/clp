import {createBrowserRouter} from "react-router";

import IngestView from "./pages/IngestPage";
import MainLayout from "./components/Layout/MainLayout";
import SearchView from "./pages/SearchPage";


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
