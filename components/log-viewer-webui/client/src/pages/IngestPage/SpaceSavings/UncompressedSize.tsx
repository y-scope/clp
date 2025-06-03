import {Nullable} from "src/typings/common";

import DetailsCard from "../Details/DetailsCard";


interface UncompressedSizeProps {
    UncompressedSize: Nullable<number>;
}

/**
 * Renders the uncompressed size statistic.
 *
 * @param props
 * @param props.uncompressedSize
 * @return
 */
const UncompressedSize = ({UncompressedSize}: UncompressedSizeProps) => {
    return (
        <DetailsCard
            stat={(UncompressedSize ?? 0).toString()}
            title={"Uncompressed Size"}/>
    );
};

export default UncompressedSize;
