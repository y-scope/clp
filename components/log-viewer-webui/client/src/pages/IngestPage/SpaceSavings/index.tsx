import {
    useCallback,
    useEffect,
    useRef,
    useState,
} from "react";

import {theme} from "antd";

import StatCard from "../../../components/StatCard";
import useRefreshIntervalStore from "../RefreshIntervalState";
import {querySql} from "../sqlConfig";
import {
    getSpaceSavingsSql,
    SpaceSavingsResp,
} from "./sql";


/**
 * Renders space savings card.
 *
 * @return
 */
const SpaceSavings = () => {
    const {refreshInterval} = useRefreshIntervalStore();
    const [compressedSize, setCompressedSize] = useState<number>(0);
    const [uncompressedSize, setUncompressedSize] = useState<number>(0);
    const {token} = theme.useToken();
    const intervalIdRef = useRef<number>(0);

    const update = useCallback(() => {
        (async () => {
            const {data: [resp]} = await querySql<SpaceSavingsResp>(getSpaceSavingsSql());
            if ("undefined" === typeof resp) {
                throw new Error();
            }
            setCompressedSize(resp.total_compressed_size);
            setUncompressedSize(resp.total_uncompressed_size);
        })().catch((error: unknown) => {
            console.error("An error occurred when fetching space savings: ", error);
        });
    }, [setCompressedSize,
        setUncompressedSize]);

    useEffect(() => {
        update();
        intervalIdRef.current = setInterval(update, refreshInterval);

        return () => {
            clearInterval(intervalIdRef.current);
        };
    }, [refreshInterval,
        update]);


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
