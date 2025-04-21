import {theme} from "antd";

import StatCard from "../../../components/StatCard";


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
            stat={DUMMY_FILES.toString()}
            statColor={token.colorTextSecondary}
            statSize={"1.3rem"}
            title={"Files"}/>
    );
};

export default Files;
