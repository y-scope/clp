import {StrictMode} from "react";
import {createRoot} from "react-dom/client";

import App from "./App";
import AntdApp from "./AntApp";

import "./index.css";

// Temporary flag to switch between old UI and new UI.
// TODO: Remove flag and related conditional logic when the new UI is finished.
let antdFlag = true;

const rootElement = document.getElementById("root");
if (null === rootElement) {
    throw new Error("Root element not found");
}

const root = createRoot(rootElement);
if (antdFlag) {
    root.render(
         <StrictMode>
             <AntdApp/>
         </StrictMode>
    );
}
else {
    root.render(
        <StrictMode>
            <App/>
        </StrictMode>
    );
}
