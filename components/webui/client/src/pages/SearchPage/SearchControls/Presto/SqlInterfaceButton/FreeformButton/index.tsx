import {EditOutlined} from "@ant-design/icons";
import {Button} from "antd";

import usePrestoSearchState from "../../../../SearchState/Presto";
import {PRESTO_SQL_INTERFACE} from "../../../../SearchState/Presto/typings";


/**
 * Renders a button to switch to Freeform SQL interface.
 *
 * @return
 */
const FreeformButton = () => {
    const setSqlInterface = usePrestoSearchState((state) => state.setSqlInterface);

    const handleClick = () => {
        setSqlInterface(PRESTO_SQL_INTERFACE.FREEFORM);
    };

    return (
        <Button
            block={true}
            color={"default"}
            icon={<EditOutlined/>}
            size={"middle"}
            variant={"filled"}
            onClick={handleClick}
        >
            Freeform
        </Button>
    );
};


export default FreeformButton;
