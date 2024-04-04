import {Meteor} from "meteor/meteor";
import React from "react";
import {render} from "react-dom";
import {Router} from "react-router";

import {createBrowserHistory} from "history";

import {App} from "/imports/ui/App.jsx";

Meteor.startup(() => {
    const routes = (
        <Router history={createBrowserHistory()}>
            <App/>
        </Router>
    );

    render(routes, document.getElementById("root"));
});
