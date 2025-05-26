import DetailsCard from "./DetailsCard";


interface FilesProps {
    numFiles: number;
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
            stat={numFiles.toString()}
            title={"Files"}/>
    );
};

export default Files;
