import DetailsCard from "./DetailsCard";


// eslint-disable-next-line no-warning-comments
// TODO: Replace with values from database once api implemented.
const DUMMY_MESSAGES = 1235844;

/**
 * Renders the messages statistic.
 *
 * @return
 */
const Messages = () => {
    return (
        <DetailsCard
            stat={DUMMY_MESSAGES.toString()}
            title={"Messages"}/>
    );
};

export default Messages;
