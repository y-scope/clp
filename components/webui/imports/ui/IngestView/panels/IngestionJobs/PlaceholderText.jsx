import Placeholder from "react-bootstrap/Placeholder";


/**
 * Renders an animated text placeholder when the text is an empty string.
 *
 * @param {object} props
 * @param {string} props.text
 * @return {React.ReactElement}
 */
const PlaceholderText = ({text}) => (
    <Placeholder
        animation={"glow"}
        as={"span"}
    >
        {text ?
            text :
            <Placeholder
                size={"sm"}
                xs={4}/>}
    </Placeholder>
);

export default PlaceholderText;
