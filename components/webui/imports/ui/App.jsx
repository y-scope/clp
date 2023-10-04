import React from "react";
import {Redirect, Route, Switch} from "react-router";

import {faFileUpload, faSearch} from "@fortawesome/free-solid-svg-icons";

import IngestView from "./IngestView/IngestView.jsx";
import SearchView from "./SearchView/SearchView.jsx";
import Sidebar from "./Sidebar/Sidebar.jsx";

import "./App.scss";

const ROUTES = [
    {
        "path": "/ingest",
        "label": "Ingest",
        "icon": faFileUpload,
        "component": IngestView,
    },
    {
        "path": "/search",
        "label": "Search",
        "icon": faSearch,
        "component": SearchView,
    },
];

export const App = () => {
    const [isSidebarStateCollapsed, setSidebarStateCollapsed] = React.useState("true" === localStorage.getItem("isSidebarCollapsed") || false);
    React.useEffect(() => {
        localStorage.setItem("isSidebarCollapsed", isSidebarStateCollapsed.toString());
    }, [isSidebarStateCollapsed]);
    const [isSidebarVisuallyCollapsed, setSidebarVisuallyCollapsed] = React.useState("true" === localStorage.getItem("isSidebarCollapsed") || false);
    const [isSidebarTransitioning, setIsSidebarTransitioning] = React.useState(false);

    const handleSidebarToggle = () => {
        setSidebarStateCollapsed(!isSidebarStateCollapsed);
        setIsSidebarTransitioning(true);
    }

    const handleSidebarTransitioned = () => {
        setSidebarVisuallyCollapsed(isSidebarStateCollapsed);
        setIsSidebarTransitioning(false);
    }

    return (
        <div className={"wrapper"}>
            <Sidebar
                isSidebarCollapsed={isSidebarStateCollapsed}
                onSidebarToggle={handleSidebarToggle}
                onSidebarTransitioned={handleSidebarTransitioned}
                routes={ROUTES}
            />
            <div style={{flex: "1 1 auto"}}>
                <div
                    className={(isSidebarStateCollapsed ? "sidebar-collapsed" : "")}
                    id="component-wrapper">
                    <Switch>
                        <Route exact path="/">
                            <Redirect to="/ingest"/>
                        </Route>
                        <Route exact path={"/ingest"}>
                            <IngestView isSidebarCollapsed={isSidebarVisuallyCollapsed}/>
                        </Route>
                        <Route exact path={"/search"}>
                            <SearchView/>
                        </Route>
                    </Switch>
                </div>
            </div>
        </div>
    );
}
