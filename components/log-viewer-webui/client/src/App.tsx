import {RouterProvider} from "react-router";

import router from "./router";

import "@ant-design/v5-patch-for-react-19";


/**
 * Renders Web UI app.
 *
 * @return
 */
const AntApp = () => {
    return <RouterProvider router={router}/>;
};

export default AntApp;
