import {Nullable} from "src/typings/common";

import DetailsCard from "./DetailsCard";


interface MessagesProps {
    numMessages: Nullable<number>;
    isLoading: boolean;
}

/**
 * Renders the messages statistic.
 *
 * @param props
 * @param props.numMessages
 * @param props.isLoading
 * @return
 */
const Messages = ({numMessages, isLoading}: MessagesProps) => {
    return (
        <DetailsCard
            isLoading={isLoading}
            stat={(numMessages ?? 0).toString()}
            title={"Messages"}/>
    );
};

export default Messages;
