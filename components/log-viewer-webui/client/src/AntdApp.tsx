import {RouterProvider} from "react-router";

import router from "./router";


/**
 * Renders Web UI app.
 *
 * @return
 */
const AntApp = () => {
    return <RouterProvider router={router}/>;
};

export default AntApp;
