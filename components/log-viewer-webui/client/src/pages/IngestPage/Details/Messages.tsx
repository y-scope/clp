import useIngestStore from "../IngestState";
import DetailsCard from "./DetailsCard";


/**
 * Renders the messages statistic.
 *
 * @return
 */
const Messages = () => {
    const messages = useIngestStore((state) => state.numMessages);
    return (
        <DetailsCard
            stat={messages.toString()}
            title={"Messages"}/>
    );
};

export default Messages;
