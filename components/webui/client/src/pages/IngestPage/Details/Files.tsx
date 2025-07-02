import {Nullable} from "src/typings/common";

import DetailsCard from "./DetailsCard";


interface FilesProps {
    numFiles: Nullable<number>;
}

/**
 * Renders the files statistic.
 *
 * @param props
 * @param props.numFiles
 * @return
 */
const Files = ({numFiles}: FilesProps) => {
    return (
        <DetailsCard
            stat={(numFiles ?? 0).toString()}
            title={"Files"}/>
    );
};

export default Files;
