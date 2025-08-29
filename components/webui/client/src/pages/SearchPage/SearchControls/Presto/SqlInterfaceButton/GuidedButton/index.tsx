import {AppstoreOutlined} from "@ant-design/icons";
import {Button} from "antd";

import usePrestoSearchState from "../../../../SearchState/Presto";
import {PRESTO_SQL_INTERFACE} from "../../../../SearchState/Presto/typings";


/**
 * Renders a button to switch to Guided SQL interface.
 *
 * @return
 */
const GuidedButton = () => {
    const setSqlInterface = usePrestoSearchState((state) => state.setSqlInterface);

    const handleClick = () => {
        setSqlInterface(PRESTO_SQL_INTERFACE.GUIDED);
    };

    return (
        <Button
            block={true}
            color={"default"}
            icon={<AppstoreOutlined/>}
            size={"middle"}
            variant={"filled"}
            onClick={handleClick}
        >
            Guided
        </Button>
    );
};

export default GuidedButton;
