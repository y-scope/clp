import useIngestStore from "../IngestState";
import DetailsCard from "./DetailsCard";


/**
 * Renders the files statistic.
 *
 * @return
 */
const Files = () => {
    const files = useIngestStore((state) => state.numFiles);
    return (
        <DetailsCard
            stat={files.toString()}
            title={"Files"}/>
    );
};

export default Files;
