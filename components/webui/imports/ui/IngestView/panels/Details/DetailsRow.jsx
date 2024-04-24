import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";


/**
 * Represents a compression stats details row.
 *
 * @param {object} props
 * @param {React.ReactNode} props.children The content in the Detail row.
 * @param {import("@fortawesome/react-fontawesome").IconProp} props.faIcon The FontAwesomeIcon to
 * display in the Detail row.
 * @param {string} props.title
 * @return {React.ReactElement}
 */
const DetailsRow = ({
    faIcon,
    children,
    title,
}) => (
    <div className={"ingest-stats-details-row"}>
        <div className={"ingest-stats-details-icon-container"}>
            <FontAwesomeIcon icon={faIcon}/>
        </div>
        <div className={"ingest-stats-details-text-container"}>
            <div className={"ingest-stats-detail"}>
                {children}
            </div>
            <span className={"ingest-desc-text"}>
                {title}
            </span>
        </div>
    </div>
);

export default DetailsRow;
