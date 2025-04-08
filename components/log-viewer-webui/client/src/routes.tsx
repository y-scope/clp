import {createBrowserRouter} from "react-router";

import MainLayout from "./components/Layout/MainLayout";
import IngestView from "./pages/IngestPage";
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
