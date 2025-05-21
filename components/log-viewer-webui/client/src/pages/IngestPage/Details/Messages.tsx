import DetailsCard from "./DetailsCard";


/**
 * Renders the messages statistic.
 *
 * @param props
 * @param props.numMessages
 * @return
 */
const Messages = ({numMessages}: {numMessages: number}) => {
    return (
        <DetailsCard
            stat={numMessages.toString()}
            title={"Messages"}/>
    );
};

export default Messages;
