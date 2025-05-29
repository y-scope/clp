import {useState} from "react";
import {
    Link,
    Outlet,
} from "react-router";

import {
    SearchOutlined,
    UploadOutlined,
} from "@ant-design/icons";
import {
    Layout,
    Menu,
    MenuProps,
} from "antd";

import styles from "./MainLayout.module.css";


const {Sider} = Layout;

type MenuItem = Required<MenuProps>["items"][number];

const SIDEBAR_MENU_ITEMS: MenuItem[] = [
    {label: <Link to={"/ingest"}>Ingest</Link>, key: "/ingest", icon: <UploadOutlined/>},
    {label: <Link to={"/search"}>Search</Link>, key: "/search", icon: <SearchOutlined/>},
];

/**
 * The main layout of web ui.
 *
 * @return
 */
const MainLayout = () => {
    const [collapsed, setCollapsed] = useState(false);

    return (
        <Layout className={styles["mainLayout"]}>
            <Sider
                collapsed={collapsed}
                collapsible={true}
                theme={"light"}
                width={150}
                onCollapse={(value) => {
                    setCollapsed(value);
                }}
            >
                <div className={styles["siderLogoContainer"]}>
                    <img
                        alt={"CLP Logo"}
                        className={styles["siderLogo"]}
                        src={"/clp-logo.png"}/>
                </div>
                <Menu
                    items={SIDEBAR_MENU_ITEMS}
                    mode={"inline"}/>
            </Sider>
            <Layout>
                <Outlet/>
            </Layout>
        </Layout>
    );
};


export default MainLayout;
