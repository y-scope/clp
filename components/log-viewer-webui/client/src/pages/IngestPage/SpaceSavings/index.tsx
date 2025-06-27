import {useQuery} from "@tanstack/react-query";
import {theme} from "antd";

import {querySql} from "../../../api/sql";
import StatCard from "../../../components/StatCard";
import {
    CLP_STORAGE_ENGINES,
    SETTINGS_STORAGE_ENGINE,
} from "../../../config";
import {fetchDatasetNames} from "../../SearchPage/SearchControls/Dataset/sql";
import CompressedSize from "./CompressedSize";
import styles from "./index.module.css";
import {
    buildMultiDatasetSpaceSavingsSql,
    getSpaceSavingsSql,
    SPACE_SAVINGS_DEFAULT,
    SpaceSavingsItem,
} from "./sql";
import UncompressedSize from "./UncompressedSize";


/**
 * Renders space savings card.
 *
 * @return
 */
const SpaceSavings = () => {
    const {token} = theme.useToken();

    const {data: datasetNames, isSuccess: isSuccessDatasetNames} = useQuery({
        queryKey: ["datasets"],
        queryFn: fetchDatasetNames,
        enabled: CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE,
    });

    const {data: spaceSavings = SPACE_SAVINGS_DEFAULT, isPending} = useQuery({
        queryKey: ["space-savings",
            datasetNames],
        queryFn: async () => {
            let sql: string;

            if (CLP_STORAGE_ENGINES.CLP === SETTINGS_STORAGE_ENGINE) {
                sql = getSpaceSavingsSql();
            } else {
                // CLP-S storage engine
                if (false === isSuccessDatasetNames) {
                    throw new Error("Dataset names are not available");
                }
                if (0 === datasetNames.length) {
                    return SPACE_SAVINGS_DEFAULT;
                }
                sql = buildMultiDatasetSpaceSavingsSql(datasetNames);
            }

            const resp = await querySql<SpaceSavingsItem[]>(sql);
            const [spaceSavingsResult] = resp.data;
            if ("undefined" === typeof spaceSavingsResult) {
                throw new Error("Space savings result does not contain data.");
            }

            return spaceSavingsResult;
        },
        enabled: CLP_STORAGE_ENGINES.CLP === SETTINGS_STORAGE_ENGINE || isSuccessDatasetNames,
    });

    const compressedSize = spaceSavings.total_compressed_size;
    const uncompressedSize = spaceSavings.total_uncompressed_size;

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
                isLoading={isPending}
                uncompressedSize={uncompressedSize}/>
            <CompressedSize
                compressedSize={compressedSize}
                isLoading={isPending}/>
        </div>
    );
};

export default SpaceSavings;
