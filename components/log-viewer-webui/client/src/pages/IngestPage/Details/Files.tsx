import DetailsCard from "./DetailsCard";


// eslint-disable-next-line no-warning-comments
// TODO: Replace with values from database once api implemented.
const DUMMY_FILES = 124;

/**
 * Renders the files statistic.
 *
 * @return
 */
const Files = () => {
    return (
        <DetailsCard
            stat={DUMMY_FILES.toString()}
            title={"Files"}/>
    );
};

export default Files;
