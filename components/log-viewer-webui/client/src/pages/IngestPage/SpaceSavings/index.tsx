import {theme} from "antd";

import StatCard from "../../../components/StatCard";


// eslint-disable-next-line no-warning-comments
// TODO: Replace with values from database once api implemented.
const DUMMY_COMPRESSED_SIZE = 1004023;
const DUMMY_UNCOMPRESSED_SIZE = 110300010;

/**
 * Presents space savings or compression ratio from the given statistics.
 *
 * @return
 */
const SpaceSavings = () => {
    const {token} = theme.useToken();
    const compressedSize = DUMMY_COMPRESSED_SIZE as number;
    const uncompressedSize = DUMMY_UNCOMPRESSED_SIZE as number;

    const spaceSavingsPercent = (0 === uncompressedSize) ?
        100 * (1 - (compressedSize / uncompressedSize)) :
        0;

    const spaceSavingsPercentText = `${spaceSavingsPercent.toFixed(2)}%`;

    return (
        <StatCard
            backgroundColor={token.colorPrimary}
            stat={spaceSavingsPercentText}
            textColor={token.colorWhite}
            title={"Space Savings"}/>
    );
};


export default SpaceSavings;
