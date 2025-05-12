import {theme} from "antd";

import StatCard from "../../../components/StatCard";
import useIngestStore from "../IngestState";


/**
 * Renders space savings card.
 *
 * @return
 */
const SpaceSavings = () => {
    const {token} = theme.useToken();
    const compressedSize = useIngestStore((state) => state.compressedSize);
    const uncompressedSize = useIngestStore((state) => state.uncompressedSize);


    const spaceSavingsPercent = (0 !== uncompressedSize) ?
        100 * (1 - (compressedSize / uncompressedSize)) :
        0;

    const spaceSavingsPercentText = `${spaceSavingsPercent.toFixed(2)}%`;

    return (
        <StatCard
            backgroundColor={token.colorPrimary}
            stat={spaceSavingsPercentText}
            statColor={token.colorWhite}
            statSize={"6rem"}
            title={"Space Savings"}
            titleColor={token.colorWhite}/>
    );
};

export default SpaceSavings;
