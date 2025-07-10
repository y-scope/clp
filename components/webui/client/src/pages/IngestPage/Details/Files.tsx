import {Nullable} from "src/typings/common";

import DetailsCard from "./DetailsCard";


interface FilesProps {
    numFiles: Nullable<number>;
    isLoading: boolean;
}

/**
 * Renders the files statistic.
 *
 * @param props
 * @param props.numFiles
 * @param props.isLoading
 * @return
 */
const Files = ({numFiles, isLoading}: FilesProps) => {
    return (
        <DetailsCard
            isLoading={isLoading}
            stat={(numFiles ?? 0).toString()}
            title={"Files"}/>
    );
};

export default Files;
