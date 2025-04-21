import styles from "./index.module.css";
import {theme} from "antd";
import StatCard from "../../../components/StatCard";

// eslint-disable-next-line no-warning-comments
// TODO: Replace with values from database once api implemented.
const DUMMY_MESSAGES = 1235844;

/**
 * Renders the messages statistic.
 *
 * @return
 */
const Messages = () => {
    const {token} = theme.useToken();
    return (
        <StatCard
            title="Messages"
            stat={DUMMY_MESSAGES.toString()}
            statSize="1.3rem"
            statColor={token.colorTextSecondary}
        />
    );
};

export default Messages;
