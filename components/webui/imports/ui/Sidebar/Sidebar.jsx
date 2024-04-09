import {NavLink} from "react-router-dom";

import {
    faAngleDoubleLeft,
    faAngleDoubleRight,
} from "@fortawesome/free-solid-svg-icons";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";

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
}) => {
    return (
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

            <div className={"flex-column sidebar-menu"}>
                {routes.map((route, i) => (false === (route.hide ?? false)) && (
                    <NavLink
                        activeClassName={"active"}
                        key={i}
                        to={route.path}
                    >
                        <div className={"sidebar-item-icon"}>
                            <FontAwesomeIcon
                                fixedWidth={true}
                                icon={route.icon}
                                size={"sm"}/>
                        </div>
                        <span className={"sidebar-item-text"}>
                            {route.label}
                        </span>
                    </NavLink>
                ))}
            </div>

            <div
                className={"sidebar-collapse-toggle"}
                onClick={onSidebarToggle}
            >
                <div className={"sidebar-collapse-icon"}>
                    {
                        isSidebarCollapsed ?
                            <FontAwesomeIcon
                                icon={faAngleDoubleRight}
                                size={"sm"}/> :
                            <FontAwesomeIcon
                                icon={faAngleDoubleLeft}
                                size={"sm"}/>
                    }
                </div>
                <span className={"sidebar-collapse-text"}>Collapse Menu</span>
            </div>
        </div>
    );
};

export default Sidebar;
