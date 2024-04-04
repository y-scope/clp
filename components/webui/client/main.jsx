import {Meteor} from "meteor/meteor";
import React from "react";
import {render} from "react-dom";
import {Router} from "react-router";

import dayjs from "dayjs";
import Duration from "dayjs/plugin/duration";
import Timezone from "dayjs/plugin/timezone";
import Utc from "dayjs/plugin/utc";
import {createBrowserHistory} from "history";

import {App} from "/imports/ui/App.jsx";


dayjs.extend(Utc);
dayjs.extend(Timezone);
dayjs.extend(Duration);

Meteor.startup(() => {
    const routes = (
        <Router history={createBrowserHistory()}>
            <App/>
        </Router>
    );

    render(routes, document.getElementById("root"));
});
