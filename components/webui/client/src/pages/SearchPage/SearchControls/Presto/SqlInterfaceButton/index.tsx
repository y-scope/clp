import {
    AppstoreOutlined,
    EditOutlined,
} from "@ant-design/icons";
import {
    ConfigProvider,
    Segmented,
    theme,
} from "antd";

import useSearchStore from "../../../SearchState";
import usePrestoSearchState from "../../../SearchState/Presto";
import {PRESTO_SQL_INTERFACE} from "../../../SearchState/Presto/typings";
import {SEARCH_UI_STATE} from "../../../SearchState/typings";
import {handleSwitchToFreeform} from "../presto-guided-search-requests";
import {handleSwitchToGuided} from "../presto-search-requests";
import styles from "./index.module.css";


/**
 * Renders the button to switch Presto SQL interface.
 *
 * @return
 */
const SqlInterfaceButton = () => {
    const {token} = theme.useToken();
    const sqlInterface = usePrestoSearchState((state) => state.sqlInterface);
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const disabled = searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
        searchUiState === SEARCH_UI_STATE.QUERYING;

    const handleChange = (value: PRESTO_SQL_INTERFACE) => {
        if (value === sqlInterface) {
            return;
        }

        if (value === PRESTO_SQL_INTERFACE.GUIDED) {
            handleSwitchToGuided();

            return;
        }

        handleSwitchToFreeform();
    };

    return (
        <div className={styles["sqlInterfaceButton"]}>
            <ConfigProvider
                theme={{
                    components: {
                        Segmented: {
                            itemSelectedBg: token.colorBorder,
                            trackBg: token.colorBorderSecondary,
                            trackPadding: 1,
                        },
                    },
                }}
            >
                <Segmented
                    block={true}
                    disabled={disabled}
                    size={"middle"}
                    value={sqlInterface}
                    options={[
                        {
                            label: "Guided",
                            value: PRESTO_SQL_INTERFACE.GUIDED,
                            icon: <AppstoreOutlined/>,
                        },
                        {
                            label: "Manual",
                            value: PRESTO_SQL_INTERFACE.FREEFORM,
                            icon: <EditOutlined/>,
                        },
                    ]}
                    onChange={(value) => {
                        handleChange(value);
                    }}/>
            </ConfigProvider>
        </div>
    );
};

export default SqlInterfaceButton;
