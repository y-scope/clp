import {NavLink} from "react-router-dom";

import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";


/**
 * Component for rendering a sidebar link.
 *
 * @param {object} props
 * @param {import("@fortawesome/react-fontawesome").IconProp} props.icon
 * @param {string} props.label
 * @param {string} [props.link]
 * @param {Function} [props.onClick]
 * @return {React.ReactElement}
 */
const SidebarButton = ({
    icon,
    label,
    link,
    onClick,
}) => {
    const children = (
        <>
            <div className={"sidebar-item-icon"}>
                <FontAwesomeIcon
                    fixedWidth={true}
                    icon={icon}
                    size={"sm"}/>
            </div>
            <span className={"sidebar-item-text"}>
                {label}
            </span>
        </>
    );

    const isNavLink = ("undefined" !== typeof link) && link.startsWith("/");
    if (isNavLink) {
        return (
            <NavLink
                activeClassName={"active"}
                to={link}
                onClick={onClick}
            >
                {children}
            </NavLink>
        );
    }

    return (
        <a
            href={link}
            rel={"noreferrer"}
            target={"_blank"}
            onClick={onClick}
        >
            {children}
        </a>
    );
};

export default SidebarButton;
