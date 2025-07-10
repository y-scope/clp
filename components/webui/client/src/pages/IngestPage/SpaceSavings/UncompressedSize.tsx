import DetailsCard from "../Details/DetailsCard";
import {formatSizeInBytes} from "../Jobs/units";


interface UncompressedSizeProps {
    uncompressedSize: number;
    isLoading: boolean;
}

/**
 * Renders the uncompressed size statistic.
 *
 * @param props
 * @param props.uncompressedSize
 * @param props.isLoading
 * @return
 */
const UncompressedSize = ({uncompressedSize, isLoading}: UncompressedSizeProps) => {
    return (
        <DetailsCard
            isLoading={isLoading}
            stat={formatSizeInBytes(uncompressedSize, false)}
            title={"Uncompressed Size"}/>
    );
};

export default UncompressedSize;
