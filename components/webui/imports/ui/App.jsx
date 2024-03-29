import React from "react";

import {faFileUpload, faSearch} from "@fortawesome/free-solid-svg-icons";
import {Redirect, Route, Switch} from "react-router";

import {LOCAL_STORAGE_KEYS} from "./constants";

import IngestView from "./IngestView/IngestView.jsx";
import SearchView from "./SearchView/SearchView.jsx";
import Sidebar from "./Sidebar/Sidebar.jsx";

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

const App = () => {
    const [isSidebarCollapsed, setSidebarStateCollapsed] = React.useState(
        "true" === localStorage.getItem(LOCAL_STORAGE_KEYS.IS_SIDEBAR_COLLAPSED),
    );

    React.useEffect(() => {
        localStorage.setItem(LOCAL_STORAGE_KEYS.IS_SIDEBAR_COLLAPSED,
            isSidebarCollapsed.toString());
    }, [isSidebarCollapsed]);

    const handleSidebarToggle = () => {
        setSidebarStateCollapsed(!isSidebarCollapsed);
    };

    return (<>
        <Sidebar
            isSidebarCollapsed={isSidebarCollapsed}
            routes={ROUTES}
            onSidebarToggle={handleSidebarToggle}
        />
        <div id={"page-container"}>
            <Switch>
                <Route exact path="/">
                    <Redirect to="/ingest"/>
                </Route>
                <Route exact path={"/ingest"}>
                    <IngestView/>
                </Route>
                <Route exact path={"/search"}>
                    <SearchView/>
                </Route>
            </Switch>
        </div>
    </>);
};

export {App};
