import DetailsCard from "./DetailsCard";


interface MessagesProps {
    numMessages: number;
}

/**
 * Renders the messages statistic.
 *
 * @param props
 * @param props.numMessages
 * @return
 */
const Messages = ({numMessages}: MessagesProps) => {
    return (
        <DetailsCard
            stat={numMessages.toString()}
            title={"Messages"}/>
    );
};

export default Messages;
