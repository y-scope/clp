import {
    Link,
    Outlet,
    useLocation,
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
import ThemeToggleMenuItem from "./ThemeToggleMenuItem";


const {Sider} = Layout;

type MenuItem = Required<MenuProps>["items"][number];

const sidebarTopMenuItems: MenuItem[] = [
    {type: "divider"},
    {label: <Link to={"/ingest"}>Ingest</Link>, key: "/ingest", icon: <UploadOutlined/>},
    {label: <Link to={"/search"}>Search</Link>, key: "/search", icon: <SearchOutlined/>},
];

const sidebarBottomMenuItems = [
    <ThemeToggleMenuItem key={"theme-toggle"}/>,
];

/**
 * The main layout of web ui.
 *
 * @return
 */
const MainLayout = () => {
    const {pathname} = useLocation();

    return (
        <Layout className={styles["mainLayout"]}>
            <Sider
                collapsedWidth={50}
                collapsible={true}
                theme={"light"}
                width={"200"}
            >
                <div className={styles["siderContainer"]}>
                    <div className={styles["siderLogoContainer"]}>
                        <img
                            alt={"CLP Logo"}
                            className={styles["siderLogo"]}
                            src={"/clp-logo.png"}/>
                    </div>
                    <Menu
                        className={styles["siderTopMenu"]}
                        items={sidebarTopMenuItems}
                        selectedKeys={[pathname]}/>
                    <Menu
                        selectable={false}
                        selectedKeys={[]}
                    >
                        {sidebarBottomMenuItems}
                    </Menu>
                </div>
            </Sider>
            <Layout>
                <Outlet/>
            </Layout>
        </Layout>
    );
};


export default MainLayout;
