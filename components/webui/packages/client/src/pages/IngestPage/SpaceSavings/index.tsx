import {useQuery} from "@tanstack/react-query";
import {CLP_STORAGE_ENGINES} from "@webui/common/config";
import {theme} from "antd";

import {DashboardCard} from "../../../components/DashboardCard";
import Stat from "../../../components/Stat";
import {SETTINGS_STORAGE_ENGINE} from "../../../config";
import {fetchDatasetNames} from "../../SearchPage/SearchControls/Dataset/sql";
import CompressedSize from "./CompressedSize";
import styles from "./index.module.css";
import {
    fetchClpSpaceSavings,
    fetchClpsSpaceSavings,
    SPACE_SAVINGS_DEFAULT,
} from "./sql";
import UncompressedSize from "./UncompressedSize";


/**
 * Renders space savings card.
 *
 * @return
 */
const SpaceSavings = () => {
    const {token} = theme.useToken();

    const {data: datasetNames = [], isSuccess: isSuccessDatasetNames} = useQuery({
        queryKey: ["datasets"],
        queryFn: fetchDatasetNames,
        enabled: CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE,
    });

    const {data: spaceSavings = SPACE_SAVINGS_DEFAULT, isPending} = useQuery({
        queryKey: [
            "space-savings",
            datasetNames,
        ],
        queryFn: async () => {
            if (CLP_STORAGE_ENGINES.CLP === SETTINGS_STORAGE_ENGINE) {
                return fetchClpSpaceSavings();
            }

            return fetchClpsSpaceSavings(datasetNames);
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
                <DashboardCard
                    backgroundColor={token.colorPrimary}
                    isLoading={isPending}
                    title={"Space Savings"}
                    titleColor={token.colorWhite}
                >
                    <Stat
                        color={token.colorWhite}
                        fontSize={"5.5rem"}
                        text={spaceSavingsPercentText}/>
                </DashboardCard>
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
