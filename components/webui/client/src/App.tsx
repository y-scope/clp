import {RouterProvider} from "react-router";

import {ConfigProvider} from "antd";

import router from "./router";
import THEME_CONFIG from "./theme";

import "@ant-design/v5-patch-for-react-19";


/**
 * Renders Web UI app.
 *
 * @return
 */
const App = () => {
    return (
        <ConfigProvider
            theme={THEME_CONFIG}
        >
            <RouterProvider router={router}/>
        </ConfigProvider>
    );
};

export default App;
