import {StrictMode} from "react";
import {createRoot} from "react-dom/client";

import AntdApp from "./AntdApp";
import App from "./App";

import "./index.css";


/* eslint-disable-next-line no-warning-comments */
// TODO: Remove flag and related logic when the new UI is fully implemented.
const antdFlag = false;

const rootElement = document.getElementById("root");
if (null === rootElement) {
    throw new Error("Root element not found");
}

const root = createRoot(rootElement);
/* eslint-disable-next-line @typescript-eslint/no-unnecessary-condition */
if (antdFlag) {
    root.render(
        <StrictMode>
            <AntdApp/>
        </StrictMode>
    );
} else {
    root.render(
        <StrictMode>
            <App/>
        </StrictMode>
    );
}
