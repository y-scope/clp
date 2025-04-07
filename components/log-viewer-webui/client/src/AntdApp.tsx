import {RouterProvider} from "react-router";

import router from "./routes";


/**
 * Renders Web UI app.
 *
 * @return
 */
const AntApp = () => {
    return <RouterProvider router={router}/>;
};

export default AntApp;
