import React from "react";
import {Redirect, Route, Switch} from "react-router";
import {faFileUpload, faSearch} from "@fortawesome/free-solid-svg-icons";
import {v4 as uuidv4} from 'uuid';

import IngestView from "./IngestView/IngestView.jsx";
import SearchView from "./SearchView/SearchView.jsx";
import Sidebar from "./Sidebar/Sidebar.jsx";

import {
    CONST_LOCAL_STORAGE_KEY_USERNAME, loginWithUsername, registerAndLoginWithUsername
} from "../api/user/client/methods";
import "./App.scss";


const ROUTES = [{
    "path": "/ingest", "label": "Ingest", "icon": faFileUpload, "component": IngestView,
}, {
    "path": "/search", "label": "Search", "icon": faSearch, "component": SearchView,
},];

export const App = () => {
    const [loggedIn, setLoggedIn] = React.useState(false);
    const [isSidebarStateCollapsed, setSidebarStateCollapsed] = React.useState("true" === localStorage.getItem("isSidebarCollapsed") || false);

    React.useEffect(async () => {
        let username = localStorage.getItem(CONST_LOCAL_STORAGE_KEY_USERNAME);
        let result;
        if (username === null) {
            username = uuidv4();
            result = await registerAndLoginWithUsername(username);
        } else {
            result = await loginWithUsername(username);
        }

        setLoggedIn(result)
    }, []);

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

    return (<div style={{display: "flex", height: "100%"}}>
        <Sidebar
            isSidebarCollapsed={isSidebarStateCollapsed}
            onSidebarToggle={handleSidebarToggle}
            onSidebarTransitioned={handleSidebarTransitioned}
            routes={ROUTES}
        />
        <div style={{flexGrow: 1, minWidth: 0}}>
            <div style={{height: "100%"}}>
                {!loggedIn ? <div className="h-100">
                    <div className="d-flex justify-content-center align-items-center h-100">
                        <div className="spinner-grow" style={{width: '3rem', height: '3rem'}} role="status">
                            <span className="visually-hidden">Loading...</span>
                        </div>
                    </div>
                </div> : <Switch>
                    <Route exact path="/">
                        <Redirect to="/ingest"/>
                    </Route>
                    <Route exact path={"/ingest"}>
                        <IngestView isSidebarCollapsed={isSidebarVisuallyCollapsed}/>
                    </Route>
                    <Route exact path={"/search"}>
                        <SearchView/>
                    </Route>
                </Switch>}
            </div>
        </div>
    </div>);
}
