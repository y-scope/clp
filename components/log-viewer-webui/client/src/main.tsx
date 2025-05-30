import {StrictMode} from "react";
import {createRoot} from "react-dom/client";

import AntdApp from "./AntdApp";

import "./index.css";


const rootElement = document.getElementById("root");
if (null === rootElement) {
    throw new Error("Root element not found");
}

const root = createRoot(rootElement);
root.render(
    <StrictMode>
        <AntdApp/>
    </StrictMode>
);
