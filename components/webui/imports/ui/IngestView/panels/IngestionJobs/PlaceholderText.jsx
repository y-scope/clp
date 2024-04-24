import Placeholder from "react-bootstrap/Placeholder";


/**
 * Renders an animated text placeholder when the text is an empty string.
 *
 * @param {object} props
 * @param {string} props.text
 * @param {boolean} props.isAlwaysVisible
 * @return {React.ReactElement}
 */
const PlaceholderText = ({
    text,
    isAlwaysVisible = true,
}) => (
    <Placeholder
        animation={"glow"}
        as={"span"}
    >
        {(0 !== text.length) ?
            text :
            (
                (isAlwaysVisible) &&
                <Placeholder
                    size={"sm"}
                    xs={4}/>
            )}
    </Placeholder>
);

export default PlaceholderText;
