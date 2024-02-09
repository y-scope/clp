import React from "react";

import {faAngleDoubleLeft, faAngleDoubleRight} from "@fortawesome/free-solid-svg-icons";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";
import {NavLink} from "react-router-dom";


/**
 * Renders a sidebar navigation component, which includes navigation links and a toggle for
 * collapsing or expanding the sidebar.
 *
 * @param {boolean} isSidebarCollapsed indicates whether the sidebar is collapsed
 * @param {Object[]} routes objects for navigation links
 * @param {function} onSidebarToggle callback to toggle the sidebar's collapsed state
 * @returns {JSX.Element}
 */
const Sidebar = ({
    isSidebarCollapsed,
    routes,
    onSidebarToggle,
}) => {
    return (
        <div
            id="sidebar"
            className={isSidebarCollapsed ? "collapsed" : ""}
        >
            <div className="brand">
                {!isSidebarCollapsed && <b style={{marginRight: "0.25rem"}}>YScope</b>}
                {isSidebarCollapsed ? <b>CLP</b> : "CLP"}
            </div>

            <div className="flex-column sidebar-menu">
                {routes.map((route, i) =>
                        (false === (route["hide"] ?? false)) && (
                            <NavLink to={route["path"]} activeClassName="active" key={i}>
                                <div className={"sidebar-item-icon"}>
                                    <FontAwesomeIcon fixedWidth={true}
                                                     size={"sm"}
                                                     icon={route["icon"]}/>
                                </div>
                                <span className={"sidebar-item-text"}>{route["label"]}</span>
                            </NavLink>
                        ),
                )}
            </div>

            <div className="sidebar-collapse-toggle" onClick={onSidebarToggle}>
                <div className={"sidebar-collapse-icon"}>{
                    isSidebarCollapsed ?
                        <FontAwesomeIcon icon={faAngleDoubleRight} size={"sm"}/> :
                        <FontAwesomeIcon icon={faAngleDoubleLeft} size={"sm"}/>
                }</div>
                <span className={"sidebar-collapse-text"}>Collapse Menu</span>
            </div>
        </div>
    );
};

export default Sidebar;
