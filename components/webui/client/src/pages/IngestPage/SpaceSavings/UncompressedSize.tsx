import {DashboardCard} from "../../../components/DashboardCard";
import Stat from "../../../components/Stat";
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
        <DashboardCard
            isLoading={isLoading}
            title={"Uncompressed Size"}
        >
            <Stat text={formatSizeInBytes(uncompressedSize, false)}/>
        </DashboardCard>
    );
};

export default UncompressedSize;
