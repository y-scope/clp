import {Nullable} from "@webui/common/utility-types";

import {DashboardCard} from "../../../components/DashboardCard";
import Stat from "../../../components/Stat";


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
        <DashboardCard
            isLoading={isLoading}
            title={"Files"}
        >
            <Stat text={(numFiles ?? 0).toString()}/>
        </DashboardCard>
    );
};

export default Files;
