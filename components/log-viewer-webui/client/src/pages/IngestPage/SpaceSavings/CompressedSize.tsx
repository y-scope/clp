import {Nullable} from "src/typings/common";

import DetailsCard from "../Details/DetailsCard";


interface CompressedSizeProps {
    compressedSize: Nullable<number>;
}

/**
 * Renders the compressed size statistic.
 *
 * @param props
 * @param props.compressedSize
 * @return
 */
const compressedSize = ({compressedSize}: CompressedSizeProps) => {
    return (
        <DetailsCard
            stat={(compressedSize ?? 0).toString()}
            title={"compressedSize Size"}/>
    );
};

export default compressedSize;