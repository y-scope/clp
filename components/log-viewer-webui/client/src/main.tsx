import {StrictMode} from "react";
import {createRoot} from "react-dom/client";

import AntdApp from "./AntdApp";
import App from "./App";

import "./index.css";

const rootElement = document.getElementById("root");
if (null === rootElement) {
    throw new Error("Root element not found");
}

/* eslint-disable-next-line no-warning-comments */
// TODO: Remove flag and related logic when the new UI is fully implemented.
const { VITE_USE_ANTD_APP } = import.meta.env;
const AppComponent = (VITE_USE_ANTD_APP === 'true') ? AntdApp : App;

const root = createRoot(rootElement);
root.render(
    <StrictMode>
        <AppComponent/>
    </StrictMode>
);
