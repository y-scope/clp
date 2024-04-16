import {Meteor} from "meteor/meteor";
import {StrictMode} from "react";
import ReactDOM from "react-dom/client";
import {Router} from "react-router";

import {createBrowserHistory} from "history";

import {App} from "/imports/ui/App";


Meteor.startup(() => {
    const root = ReactDOM.createRoot(document.getElementById("root"));

    root.render(
        <StrictMode>
            <Router history={createBrowserHistory()}>
                <App/>
            </Router>
        </StrictMode>
    );
});
