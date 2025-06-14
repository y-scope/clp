import {RouterProvider} from "react-router";

import {ConfigProvider} from "antd";

import router from "./router";
import THEME_CONFIG, {useThemeStore} from "./theme";

import "@ant-design/v5-patch-for-react-19";


/**
 * Renders Web UI app.
 *
 * @return
 */
const AntApp = () => {
    const {algorithm} = useThemeStore();

    return (
        <ConfigProvider
            theme={{
                ...THEME_CONFIG,
                algorithm: algorithm,
            }}
        >
            <RouterProvider router={router}/>
        </ConfigProvider>
    );
};

export default AntApp;
