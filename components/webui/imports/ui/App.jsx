import React from "react";
import {Redirect, Route, Switch} from "react-router";
import {faSearch} from "@fortawesome/free-solid-svg-icons";

import SearchView from "./SearchView/SearchView.jsx";
import Sidebar from "./Sidebar/Sidebar.jsx";

import {login} from "../api/user/client/methods";
import "./App.scss";
import LOCAL_STORAGE_KEYS from "./constants/LOCAL_STORAGE_KEYS";


const ROUTES = [
    {path: "/search", label: "Search", icon: faSearch, component: SearchView},
];

export const App = () => {
    const [loggedIn, setLoggedIn] = React.useState(false);
    const [isSidebarStateCollapsed, setSidebarStateCollapsed] = React.useState(
        "true" === localStorage.getItem(LOCAL_STORAGE_KEYS.IS_SIDEBAR_COLLAPSED)
    );

    React.useEffect(async () => {
        const result = await login()
        setLoggedIn(result)
    }, []);

    React.useEffect(() => {
        localStorage.setItem(LOCAL_STORAGE_KEYS.IS_SIDEBAR_COLLAPSED, isSidebarStateCollapsed.toString());
    }, [isSidebarStateCollapsed]);
    const [isSidebarVisuallyCollapsed, setSidebarVisuallyCollapsed] = React.useState(
        "true" === localStorage.getItem(LOCAL_STORAGE_KEYS.IS_SIDEBAR_COLLAPSED)
    );

    const handleSidebarToggle = () => {
        setSidebarStateCollapsed(!isSidebarStateCollapsed);
    }

    const handleSidebarTransitioned = () => {
        setSidebarVisuallyCollapsed(isSidebarStateCollapsed);
    }

    return (<div style={{display: "flex", height: "100%"}}>
        <Sidebar
            isSidebarCollapsed={isSidebarStateCollapsed}
            routes={ROUTES}
            onSidebarToggle={handleSidebarToggle}
            onSidebarTransitioned={handleSidebarTransitioned}
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
                        <Redirect to="/search"/>
                    </Route>
                    <Route exact path={"/search"}>
                        <SearchView/>
                    </Route>
                </Switch>}
            </div>
        </div>
    </div>);
}
