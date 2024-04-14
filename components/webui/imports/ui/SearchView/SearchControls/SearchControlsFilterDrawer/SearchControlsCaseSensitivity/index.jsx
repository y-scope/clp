import Col from "react-bootstrap/Col";
import Form from "react-bootstrap/Form";
import Row from "react-bootstrap/Row";

import SearchControlsFilterLabel from "../SearchControlsFilterLabel";
import SearchControlsCaseSensitivityCheck from "./SearchControlsCaseSensitivityCheck";


/**
 * Represents a component for selecting case sensitivity.
 *
 * @param {object} props
 * @param {boolean} props.ignoreCase
 * @param {Function} props.setIgnoreCase
 * @return {React.ReactElement}
 */
const SearchControlsCaseSensitivity = ({
    ignoreCase,
    setIgnoreCase,
}) => {
    /**
     * Handles case sensitivity change.
     *
     * @param {InputEvent} event
     */
    const handleCaseSensitivityChange = (event) => {
        setIgnoreCase("true" === event.target.value);
    };

    return (
        <Form.Group as={Row}>
            <SearchControlsFilterLabel>
                Case sensitivity
            </SearchControlsFilterLabel>
            <Col>
                <SearchControlsCaseSensitivityCheck
                    checked={true === ignoreCase}
                    label={"Insensitive"}
                    value={true}
                    onChange={handleCaseSensitivityChange}/>
                <SearchControlsCaseSensitivityCheck
                    checked={false === ignoreCase}
                    label={"Sensitive"}
                    value={false}
                    onChange={handleCaseSensitivityChange}/>
            </Col>
        </Form.Group>
    );
};

export default SearchControlsCaseSensitivity;
