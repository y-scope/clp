import DetailsCard from "../Details/DetailsCard";
import {formatSizeInBytes} from "../Jobs/units";


interface CompressedSizeProps {
    compressedSize: number;
}

/**
 * Renders the compressed size statistic.
 *
 * @param props
 * @param props.compressedSize
 * @return
 */
const CompressedSize = ({compressedSize}: CompressedSizeProps) => {
    return (
        <DetailsCard
            stat={formatSizeInBytes(compressedSize, false)}
            title={"Compressed Size"}/>
    );
};

export default CompressedSize;
