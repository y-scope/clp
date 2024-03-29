import React from "react";

import {createBrowserHistory} from "history";
import {Meteor} from "meteor/meteor";
import {render} from "react-dom";
import {Router, Switch} from "react-router";

import {App} from "/imports/ui/App.jsx";

import "chartjs-adapter-dayjs-4/dist/chartjs-adapter-dayjs-4.esm";

import dayjs from "dayjs";
import Duration from "dayjs/plugin/duration";
import RelativeTime from "dayjs/plugin/relativeTime";
import Timezone from "dayjs/plugin/timezone";
import Utc from "dayjs/plugin/utc";

import {
    BarElement,
    Chart as ChartJS,
    LinearScale,
    TimeScale,
    Tooltip,
} from "chart.js";
import zoomPlugin from "chartjs-plugin-zoom";

dayjs.extend(Utc);
dayjs.extend(Timezone);
dayjs.extend(Duration);
dayjs.extend(RelativeTime);

ChartJS.register(
    TimeScale,
    LinearScale,
    BarElement,
    Tooltip,
    zoomPlugin
);

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
