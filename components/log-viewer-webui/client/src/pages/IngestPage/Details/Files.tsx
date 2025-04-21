import StatCard from "../../../components/StatCard";
import styles from "./index.module.css";
import {theme} from "antd";

// eslint-disable-next-line no-warning-comments
// TODO: Replace with values from database once api implemented.
const DUMMY_FILES = 124;

/**
 * Renders the files statistic.
 *
 * @return
 */
const Files = () => {
    const {token} = theme.useToken();
    return (
        <StatCard
            title="Files"
            stat={DUMMY_FILES.toString()}
            statSize="1.3rem"
            statColor={token.colorTextSecondary}
        />
    );
};

export default Files;
