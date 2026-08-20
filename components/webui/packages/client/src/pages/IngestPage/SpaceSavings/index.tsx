import {useQuery} from "@tanstack/react-query";
import {theme} from "antd";

import {apiClient} from "../../../api/search";
import {DashboardCard} from "../../../components/DashboardCard";
import Stat from "../../../components/Stat";
import CompressedSize from "./CompressedSize";
import styles from "./index.module.css";
import UncompressedSize from "./UncompressedSize";


/**
 * Default values for space savings when no data is available.
 */
const SPACE_SAVINGS_DEFAULT = {
    total_compressed_size: 0,
    total_uncompressed_size: 0,
};


/**
 * Renders space savings card.
 *
 * @return
 */
const SpaceSavings = () => {
    const {token} = theme.useToken();

    const {data: spaceSavings = SPACE_SAVINGS_DEFAULT, isPending} = useQuery({
        queryKey: ["space-savings"],
        queryFn: async () => {
            // eslint-disable-next-line new-cap
            const {data, response} = await apiClient.GET("/metadata/space_savings", {});
            if ("undefined" === typeof data) {
                throw new Error(`Failed to fetch space savings: HTTP ${response.status}`);
            }

            return data;
        },
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
