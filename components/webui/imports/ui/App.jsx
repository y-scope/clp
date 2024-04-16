import {
    useEffect,
    useState,
} from "react";
import {
    Redirect,
    Route,
    Switch,
} from "react-router";

import {
    faFileUpload,
    faSearch,
} from "@fortawesome/free-solid-svg-icons";

import {LOCAL_STORAGE_KEYS} from "./constants";
import IngestView from "./IngestView/IngestView";
import SearchView from "./SearchView/SearchView";
import Sidebar from "./Sidebar/Sidebar";

import "./App.scss";


const ROUTES = [
    {
        path: "/ingest",
        label: "Ingest",
        icon: faFileUpload,
        component: IngestView,
    },
    {
        path: "/search",
        label: "Search",
        icon: faSearch,
        component: SearchView,
    },
];

/**
 * Represents the top-level application component.
 *
 * @return {React.ReactElement} The rendered App component.
 */
const App = () => {
    const [isSidebarCollapsed, setIsSidebarCollapsed] = useState(
        "true" === localStorage.getItem(LOCAL_STORAGE_KEYS.IS_SIDEBAR_COLLAPSED),
    );

    useEffect(() => {
        localStorage.setItem(
            LOCAL_STORAGE_KEYS.IS_SIDEBAR_COLLAPSED,
            isSidebarCollapsed.toString()
        );
    }, [isSidebarCollapsed]);

    const handleSidebarToggle = () => {
        setIsSidebarCollapsed(false === isSidebarCollapsed);
    };

    return (
        <>
            <Sidebar
                isSidebarCollapsed={isSidebarCollapsed}
                routes={ROUTES}
                onSidebarToggle={handleSidebarToggle}/>
            <div id={"page-container"}>
                <Switch>
                    <Route
                        exact={true}
                        path={"/"}
                    >
                        <Redirect to={"/ingest"}/>
                    </Route>
                    <Route
                        exact={true}
                        path={"/ingest"}
                    >
                        <IngestView/>
                    </Route>
                    <Route
                        exact={true}
                        path={"/search"}
                    >
                        <SearchView/>
                    </Route>
                </Switch>
            </div>
        </>
    );
};

export {App};
