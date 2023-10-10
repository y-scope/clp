import React from "react";
import {Redirect, Route, Switch} from "react-router";
import {Meteor} from "meteor/meteor";
import {faFileUpload, faSearch} from "@fortawesome/free-solid-svg-icons";
import {v4 as uuidv4} from 'uuid';

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

// TODO: implement a full-fledged registration sys
const CONST_LOCAL_STORAGE_KEY_USERNAME = 'username';
const CONST_DUMMY_PASSWORD = 'DummyPassword';

let LoginRetryCount = 0;
const CONST_MAX_LOGIN_RETRY = 3;

const registerAndLoginWithUsername = username => {
    Meteor.call('user.create', {username, password: CONST_DUMMY_PASSWORD},
        (error) => {
            if (error) {
                console.log('create user error', error);
                return;
            }
            localStorage.setItem(CONST_LOCAL_STORAGE_KEY_USERNAME, username);
            loginWithUsername(username);
        });
};

const loginWithUsername = username => {
    Meteor.loginWithPassword(username, CONST_DUMMY_PASSWORD,
        (error) => {
            if (error) {
                console.log('login error', error, 'LOGIN_RETRY_COUNT:', LoginRetryCount);
                if (LoginRetryCount < CONST_MAX_LOGIN_RETRY) {
                    LoginRetryCount++;
                    registerAndLoginWithUsername(username);
                }
            }
        });
};

export const App = () => {
    React.useEffect(() => {
        let username = localStorage.getItem(CONST_LOCAL_STORAGE_KEY_USERNAME);
        if (username === null) {
            username = uuidv4();
            registerAndLoginWithUsername(username);
        } else {
            loginWithUsername(username);
        }
    }, []);

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
