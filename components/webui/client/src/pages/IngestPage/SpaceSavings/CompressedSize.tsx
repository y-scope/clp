import {DashboardCard} from "../../../components/DashboardCard";
import Stat from "../../../components/Stat";
import {formatSizeInBytes} from "../Jobs/units";


interface CompressedSizeProps {
    compressedSize: number;
    isLoading: boolean;
}

/**
 * Renders the compressed size statistic.
 *
 * @param props
 * @param props.compressedSize
 * @param props.isLoading
 * @return
 */
const CompressedSize = ({compressedSize, isLoading}: CompressedSizeProps) => {
    return (
        <DashboardCard
            isLoading={isLoading}
            title={"Compressed Size"}
        >
            <Stat>
                {formatSizeInBytes(compressedSize, false)}
            </Stat>
        </DashboardCard>
    );
};

export default CompressedSize;
