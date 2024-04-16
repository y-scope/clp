import Form from "react-bootstrap/Form";


/**
 * Renders a label for a search filter control.
 *
 * @param {object} props
 * @return {React.ReactElement}
 */
const SearchControlsFilterLabel = (props) => (
    <Form.Label
        {...props}
        className={"search-filter-control-label text-nowrap"}
        column={"sm"}
        md={1}/>
);

export default SearchControlsFilterLabel;
