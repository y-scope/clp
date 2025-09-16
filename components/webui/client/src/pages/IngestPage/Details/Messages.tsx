import {Nullable} from "@webui/common/utility-types";

import {DashboardCard} from "../../../components/DashboardCard";
import Stat from "../../../components/Stat";


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
        <DashboardCard
            isLoading={isLoading}
            title={"Messages"}
        >
            <Stat text={(numMessages ?? 0).toString()}/>
        </DashboardCard>
    );
};

export default Messages;
