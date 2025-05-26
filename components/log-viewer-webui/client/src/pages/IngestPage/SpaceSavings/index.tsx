import {
    useCallback,
    useEffect,
    useRef,
    useState,
} from "react";

import {theme} from "antd";

import StatCard from "../../../components/StatCard";
import {SET_INTERVAL_INVALID_ID} from "../../../typings/time";
import useIngestStatsStore from "../ingestStatsStore";
import {querySql} from "../sqlConfig";
import {
    getSpaceSavingsSql,
    SpaceSavingsResp,
} from "./sql";


/**
 * Default state for space savings.
 */
const SPACE_SAVINGS_DEFAULT = Object.freeze({
    compressedSize: 0,
    uncompressedSize: 0,
});


/**
 * Renders space savings card.
 *
 * @return
 */
const SpaceSavings = () => {
    const {refreshInterval} = useIngestStatsStore();
    const [compressedSize, setCompressedSize] =
        useState<number>(SPACE_SAVINGS_DEFAULT.compressedSize);
    const [uncompressedSize, setUncompressedSize] =
        useState<number>(SPACE_SAVINGS_DEFAULT.uncompressedSize);
    const {token} = theme.useToken();
    const intervalIdRef = useRef<ReturnType<typeof setInterval>>(SET_INTERVAL_INVALID_ID);

    const fetchSpaceSavingsStats = useCallback(async () => {
        const {data: [resp]} = await querySql<SpaceSavingsResp>(getSpaceSavingsSql());
        if ("undefined" === typeof resp) {
            throw new Error("Space savings response is undefined");
        }
        setCompressedSize(resp.total_compressed_size);
        setUncompressedSize(resp.total_uncompressed_size);
    }, []);

    useEffect(() => {
        // eslint-disable-next-line no-void
        void fetchSpaceSavingsStats();
        intervalIdRef.current = setInterval(fetchSpaceSavingsStats, refreshInterval);

        return () => {
            clearInterval(intervalIdRef.current);
        };
    }, [
        refreshInterval,
        fetchSpaceSavingsStats,
    ]);


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
