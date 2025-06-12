import {
    useCallback,
    useEffect,
    useState,
} from "react";

import {theme} from "antd";

import StatCard from "../../../components/StatCard";
import useIngestStatsStore from "../ingestStatsStore";
import {querySql} from "../sqlConfig";
import CompressedSize from "./CompressedSize";
import styles from "./index.module.css";
import {
    getSpaceSavingsSql,
    SpaceSavingsResp,
} from "./sql";
import UncompressedSize from "./UncompressedSize";


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

    const fetchSpaceSavingsStats = useCallback(() => {
        querySql<SpaceSavingsResp>(getSpaceSavingsSql())
            .then((resp) => {
                const [spaceSavings] = resp.data;
                if ("undefined" === typeof spaceSavings) {
                    throw new Error("Space savings response is undefined");
                }
                setCompressedSize(spaceSavings.total_compressed_size);
                setUncompressedSize(spaceSavings.total_uncompressed_size);
            })
            .catch((e: unknown) => {
                console.error("Failed to fetch space savings stats", e);
            });
    }, []);

    useEffect(() => {
        fetchSpaceSavingsStats();
        const intervalId = setInterval(fetchSpaceSavingsStats, refreshInterval);

        return () => {
            clearInterval(intervalId);
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
        <div className={styles["spaceSavingsGrid"]}>
            <div className={styles["spaceSavingsCard"]}>
                <StatCard
                    backgroundColor={token.colorPrimary}
                    stat={spaceSavingsPercentText}
                    statColor={token.colorWhite}
                    statSize={"5.5rem"}
                    title={"Space Savings"}
                    titleColor={token.colorWhite}/>
            </div>
            <UncompressedSize uncompressedSize={uncompressedSize}/>
            <CompressedSize compressedSize={compressedSize}/>
        </div>
    );
};

export default SpaceSavings;
