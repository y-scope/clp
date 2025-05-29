import {Nullable} from "src/typings/common";

import DetailsCard from "./DetailsCard";


interface MessagesProps {
    numMessages: Nullable<number>;
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
            stat={(numMessages ?? 0).toString()}
            title={"Messages"}/>
    );
};

export default Messages;
