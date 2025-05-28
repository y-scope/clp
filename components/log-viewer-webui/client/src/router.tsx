import {createBrowserRouter} from "react-router";

import MainLayout from "./components/Layout/MainLayout";
import IngestPage from "./pages/IngestPage";
import SearchPage from "./pages/SearchPage";


const router = createBrowserRouter([
    {
        path: "/",
        Component: MainLayout,
        children: [
            {path: "ingest", Component: IngestPage},
            {path: "search", Component: SearchPage},
        ],
    },
]);


export default router;
