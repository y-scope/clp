import React from "react";

import {createBrowserHistory} from "history";
import {Meteor} from "meteor/meteor";
import {render} from "react-dom";
import {Router, Switch} from "react-router";

import {App} from "/imports/ui/App.jsx";

Meteor.startup(() => {
    const routes = (
        <Router history={createBrowserHistory()}>
            <App/>
        </Router>
    );

    render(routes, document.getElementById("root"));
});
