import React from "react";
import {RouterProvider} from "react-router";

import router from "./routes/routes";


/**
 * Renders Web UI app.
 *
 * @return
 */
const AntApp = () => {
    return <RouterProvider router={router}/>;
};

export default AntApp;
