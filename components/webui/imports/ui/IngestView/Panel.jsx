import Col from "react-bootstrap/Col";
import Row from "react-bootstrap/Row";

import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";


/**
 * Represents a Panel component.
 *
 * @param {object} props
 * @param {React.ReactNode} props.children The content of the Panel.
 * @param {import("@fortawesome/react-fontawesome").IconProp} props.faIcon The FontAwesomeIcon to
 * display in the Panel.
 * @param {string} props.title
 * @param {object} props.rootColProps
 * @return {React.ReactElement}
 */
const Panel = ({
    children,
    faIcon,
    title,
    ...rootColProps
}) => (
    <Col {...rootColProps}>
        <div className={"panel"}>
            <Row>
                <Col>
                    <h1 className={"panel-h1"}>
                        {title}
                    </h1>
                </Col>
                <Col xs={"auto"}>
                    <FontAwesomeIcon
                        className={"panel-icon"}
                        icon={faIcon}/>
                </Col>
                <Row>
                    <Col>
                        {children}
                    </Col>
                </Row>
            </Row>
        </div>
    </Col>
);

export default Panel;
