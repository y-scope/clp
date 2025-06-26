import DetailsCard from "../Details/DetailsCard";
import {formatSizeInBytes} from "../Jobs/units";


interface CompressedSizeProps {
    compressedSize: number;
    isLoading?: boolean;
}

/**
 * Renders the compressed size statistic.
 *
 * @param props
 * @param props.compressedSize
 * @param props.isLoading
 * @return
 */
const CompressedSize = ({compressedSize, isLoading = false}: CompressedSizeProps) => {
    return (
        <DetailsCard
            isLoading={isLoading}
            stat={formatSizeInBytes(compressedSize, false)}
            title={"Compressed Size"}/>
    );
};

export default CompressedSize;
