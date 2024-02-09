import React from "react";

import {faFileUpload, faSearch} from "@fortawesome/free-solid-svg-icons";
import {Redirect, Route, Switch} from "react-router";

import {login} from "../api/user/client/methods";
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
    const [loggedIn, setLoggedIn] = React.useState(false);
    const [isSidebarCollapsed, setSidebarStateCollapsed] = React.useState(
        "true" === localStorage.getItem(LOCAL_STORAGE_KEYS.IS_SIDEBAR_COLLAPSED),
    );

    React.useEffect(async () => {
        const result = await login();
        setLoggedIn(result);
    }, []);

    React.useEffect(() => {
        localStorage.setItem(LOCAL_STORAGE_KEYS.IS_SIDEBAR_COLLAPSED,
            isSidebarCollapsed.toString());
    }, [isSidebarCollapsed]);

    const handleSidebarToggle = () => {
        setSidebarStateCollapsed(!isSidebarCollapsed);
    };

    const Spinner = () => <div className="h-100">
        <div className="d-flex justify-content-center align-items-center h-100">
            <div className="spinner-grow"
                 style={{
                     width: "3rem",
                     height: "3rem",
                 }} role="status">
                <span className="visually-hidden">Loading...</span>
            </div>
        </div>
    </div>;

    const Routes = () => <Switch>
        <Route exact path="/">
            <Redirect to="/ingest"/>
        </Route>
        <Route exact path={"/ingest"}>
            <IngestView/>
        </Route>
        <Route exact path={"/search"}>
            <SearchView/>
        </Route>
    </Switch>;

    return (<div style={{
        display: "flex",
        height: "100%",
    }}>
        <Sidebar
            isSidebarCollapsed={isSidebarCollapsed}
            routes={ROUTES}
            onSidebarToggle={handleSidebarToggle}
        />
        <div style={{
            flexGrow: 1,
            minWidth: 0,
        }}>
            <div style={{height: "100%"}}>
                {!loggedIn ? <Spinner/> : <Routes/>}
            </div>
        </div>
    </div>);
};

export {App};
