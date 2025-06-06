import DetailsCard from "../Details/DetailsCard";
import {formatSizeInBytes} from "../Jobs/units";


interface UncompressedSizeProps {
    uncompressedSize: number;
}

/**
 * Renders the uncompressed size statistic.
 *
 * @param props
 * @param props.uncompressedSize
 * @return
 */
const UncompressedSize = ({uncompressedSize}: UncompressedSizeProps) => {
    return (
        <DetailsCard
            stat={formatSizeInBytes(uncompressedSize, false)}
            title={"Uncompressed Size"}/>
    );
};

export default UncompressedSize;
