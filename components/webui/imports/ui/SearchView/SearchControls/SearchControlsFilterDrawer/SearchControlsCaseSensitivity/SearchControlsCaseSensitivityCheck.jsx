import Form from "react-bootstrap/Form";


/**
 * Renders a case sensitivity checkbox.
 *
 * @param {object} props
 * @param {string} props.label
 * @param {object} props.rest
 * @return {React.ReactElement}
 */
const SearchControlsCaseSensitivityCheck = ({
    label,
    ...rest
}) => (
    <Form.Check
        {...rest}
        className={"mt-1"}
        inline={true}
        label={label}
        name={"case-sensitivity"}
        type={"radio"}/>
);

export default SearchControlsCaseSensitivityCheck;
