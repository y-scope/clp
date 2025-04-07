import {useState} from "react";

import {
    SearchOutlined,
    UploadOutlined,
} from "@ant-design/icons";
import {
    Layout,
    Menu,
    MenuProps,
} from "antd";

import "./MainLayout.css";


const {Sider} = Layout;

type MenuItem = Required<MenuProps>["items"][number];

const items: MenuItem[] = [
    {label: "Ingest", key: "/ingest", icon: <UploadOutlined/>},
    {label: "Search", key: "/search", icon: <SearchOutlined/>},
];

/**
 * The main layout of web ui.
 *
 * @return
 */
const MainLayout = () => {
    const [collapsed, setCollapsed] = useState(false);

    return (
        <Layout className={"main-layout"}>
            <Sider
                collapsed={collapsed}
                collapsible={true}
                theme={"light"}
                width={"150"}
                onCollapse={(value) => {
                    setCollapsed(value);
                }}
            >
                <div className={"sider-logo-container"}>
                    <img
                        alt={"CLP Logo"}
                        className={"sider-logo"}
                        src={"/clp-logo.png"}/>
                </div>
                <Menu
                    items={items}
                    mode={"inline"}/>
            </Sider>
        </Layout>
    );
};

export default MainLayout;
