import DetailsCard from "./DetailsCard";


/**
 * Renders the files statistic.
 *
 * @param props
 * @param props.numFiles
 * @return
 */
const Files = ({numFiles}: {numFiles: number}) => {
    return (
        <DetailsCard
            stat={numFiles.toString()}
            title={"Files"}/>
    );
};

export default Files;
