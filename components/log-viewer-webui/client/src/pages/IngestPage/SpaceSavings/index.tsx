import {useQuery} from "@tanstack/react-query";

import {theme} from "antd";

import StatCard from "../../../components/StatCard";
import {querySql} from "../../../api/sql";
import {fetchDatasetNames} from "../../SearchPage/SearchControls/Dataset/sql";
import useIngestStatsStore from "../ingestStatsStore";
import CompressedSize from "./CompressedSize";
import styles from "./index.module.css";
import {
    buildMultiDatasetSpaceSavingsSql,
    SpaceSavingsItem,
} from "./sql";
import UncompressedSize from "./UncompressedSize";


/**
 * Default state for space savings.
 */
const SPACE_SAVINGS_DEFAULT: SpaceSavingsItem = {
    total_compressed_size: 0,
    total_uncompressed_size: 0,
};


/**
 * Renders space savings card.
 *
 * @return
 */
const SpaceSavings = () => {
    const {refreshInterval} = useIngestStatsStore();
    const {token} = theme.useToken();

    const {data: datasetNames, isSuccess: isSuccessDatasetNames} = useQuery({
        queryKey: ["datasets"],
        queryFn: fetchDatasetNames,
        staleTime: refreshInterval,
    });

    const {data: spaceSavings, isPending} = useQuery({
        queryKey: ["space-savings", datasetNames],
        queryFn: async () => {
            if (false === isSuccessDatasetNames) {
                throw new Error("Dataset names are not available");
            }
            if (0 === datasetNames.length) {
                return SPACE_SAVINGS_DEFAULT;
            }
            const sql = buildMultiDatasetSpaceSavingsSql(datasetNames);
            const resp = await querySql<SpaceSavingsItem[]>(sql);
            const [spaceSavingsResult] = resp.data;
            if ("undefined" === typeof spaceSavingsResult) {
                throw new Error("Space savings response is undefined");
            }

            return spaceSavingsResult;
        },
        enabled: isSuccessDatasetNames,
    });

    const compressedSize = spaceSavings?.total_compressed_size ?? 0;
    const uncompressedSize = spaceSavings?.total_uncompressed_size ?? 0;

    const spaceSavingsPercent = (0 !== uncompressedSize) ?
        100 * (1 - (compressedSize / uncompressedSize)) :
        0;

    const spaceSavingsPercentText = `${spaceSavingsPercent.toFixed(2)}%`;

    return (
        <div className={styles["spaceSavingsGrid"]}>
            <div className={styles["spaceSavingsCard"]}>
                <StatCard
                    backgroundColor={token.colorPrimary}
                    isLoading={isPending}
                    stat={spaceSavingsPercentText}
                    statColor={token.colorWhite}
                    statSize={"5.5rem"}
                    title={"Space Savings"}
                    titleColor={token.colorWhite}/>
            </div>
            <UncompressedSize
                uncompressedSize={uncompressedSize}
                isLoading={isPending}/>
            <CompressedSize
                compressedSize={compressedSize}
                isLoading={isPending}/>
        </div>
    );
};

export default SpaceSavings;
