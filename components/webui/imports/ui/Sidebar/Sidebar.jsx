import React from "react";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";
import {
    faAngleDoubleLeft,
    faAngleDoubleRight,
    faSignOutAlt
} from "@fortawesome/free-solid-svg-icons";
import {NavLink} from "react-router-dom";

import "./Sidebar.scss";

const Sidebar = ({isSidebarCollapsed, onSidebarToggle, onSidebarTransitioned, routes}) => {
    return (
        <div
            id="sidebar"
            className={isSidebarCollapsed ? "collapsed" : ""}
            onTransitionEnd={onSidebarTransitioned}
        >
            <div className="brand">
                {!isSidebarCollapsed && <b style={{marginRight: "0.25rem"}}>YScope</b>}
                {isSidebarCollapsed ? <b>CLP</b> : "CLP"}
            </div>

            <div className="flex-column sidebar-menu">
                {routes.map((route, i) =>
                        (false === (route["hide"] ?? false)) && (
                            <NavLink to={route["path"]} activeClassName="active" key={i}>
                                <div className={"sidebar-item-icon"}><FontAwesomeIcon fixedWidth={true}
                                                                                      size={"sm"}
                                                                                      icon={route["icon"]}/>
                                </div>
                                <span className={"sidebar-item-text"}>{route["label"]}</span>
                            </NavLink>
                        )
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
