import {faFileUpload, faSearch} from "@fortawesome/free-solid-svg-icons";
import React from "react";
import {Hello} from "./Hello.jsx";
import {Info} from "./Info.jsx";
import {Redirect, Route, Switch} from "react-router";
import Sidebar from "./Sidebar/Sidebar.jsx";

import "./App.scss";

const ROUTES = [
    {
        "path": "/ingest",
        "label": "Ingest",
        "icon": faFileUpload,
        "component": Hello,
    },
    {
        "path": "/search",
        "label": "Search",
        "icon": faSearch,
        "component": Info,
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
                            <Hello/>
                        </Route>
                        <Route exact path={"/search"}>
                            <Info/>
                        </Route>
                    </Switch>
                </div>
            </div>
        </div>
    );
}
