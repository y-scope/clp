import React from "react";
import {Meteor} from "meteor/meteor";
import {render} from "react-dom";
import {App} from "/imports/ui/App.jsx";
import {Router, Switch} from "react-router";
import {createBrowserHistory} from "history";

Meteor.startup(() => {
    const routes = (
        <Router history={createBrowserHistory()}>
            <Switch>
                <App/>
            </Switch>
        </Router>
    );

    render(routes, document.getElementById("react-target"));
});
