import {Meteor} from "meteor/meteor";
import React from "react";
import {render} from "react-dom";
import {Router} from "react-router";

import {
    BarElement,
    Chart as ChartJs,
    LinearScale,
    TimeScale,
    Tooltip,
} from "chart.js";
import zoomPlugin from "chartjs-plugin-zoom";
import dayjs from "dayjs";
import Duration from "dayjs/plugin/duration";
import Timezone from "dayjs/plugin/timezone";
import Utc from "dayjs/plugin/utc";
import {createBrowserHistory} from "history";

import {App} from "/imports/ui/App.jsx";

import "chartjs-adapter-dayjs-4/dist/chartjs-adapter-dayjs-4.esm";


dayjs.extend(Utc);
dayjs.extend(Timezone);
dayjs.extend(Duration);

ChartJs.register(
    TimeScale,
    LinearScale,
    BarElement,
    Tooltip,
    zoomPlugin
);

Meteor.startup(() => {
    const routes = (
        <Router history={createBrowserHistory()}>
            <App/>
        </Router>
    );

    render(routes, document.getElementById("root"));
});
