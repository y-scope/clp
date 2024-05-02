import {
    faAngleDoubleLeft,
    faAngleDoubleRight,
    faCircleInfo,
    faMessage,
} from "@fortawesome/free-solid-svg-icons";

import SidebarButton from "./SidebarButton";

import "./Sidebar.scss";


/**
 * Renders a sidebar navigation component, which includes navigation links and a toggle for
 * collapsing or expanding the sidebar.
 *
 * @param {object} props
 * @param {boolean} props.isSidebarCollapsed indicates whether the sidebar is collapsed
 * @param {object[]} props.routes objects for navigation links
 * @param {Function} props.onSidebarToggle callback to toggle the sidebar's collapsed state
 * @return {React.ReactElement}
 */
const Sidebar = ({
    isSidebarCollapsed,
    routes,
    onSidebarToggle,
}) => (
    <div
        id={"sidebar"}
        className={isSidebarCollapsed ?
            "collapsed" :
            ""}
    >
        <div className={"brand"}>
            {!isSidebarCollapsed && <b style={{marginRight: "0.25rem"}}>YScope</b>}
            {isSidebarCollapsed ?
                <b>CLP</b> :
                "CLP"}
        </div>

        <div className={"flex-grow-1 sidebar-menu"}>
            {routes.map((route, i) => (false === (route.hide ?? false)) && (
                <SidebarButton
                    icon={route.icon}
                    key={i}
                    label={route.label}
                    link={route.path}/>
            ))}
        </div>

        <div className={"flex-grow-0 sidebar-menu"}>
            <SidebarButton
                icon={faCircleInfo}
                label={"Documentation"}
                link={"https://docs.yscope.com/clp/main/"}/>
            <SidebarButton
                icon={faMessage}
                label={"Get Support"}
                link={Meteor.settings.public.SupportUrl}/>
            <SidebarButton
                label={"Collapse Menu"}
                icon={isSidebarCollapsed ?
                    faAngleDoubleRight :
                    faAngleDoubleLeft}
                onClick={onSidebarToggle}/>
        </div>
    </div>
);

export default Sidebar;
