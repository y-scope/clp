import {useCallback} from "react";

import {AppstoreOutlined, EditOutlined} from "@ant-design/icons";
import {Tabs} from "antd";

import useSearchStore from "../../../SearchState";
import usePrestoSearchState from "../../../SearchState/Presto";
import {PRESTO_SQL_INTERFACE} from "../../../SearchState/Presto/typings";
import {SEARCH_UI_STATE} from "../../../SearchState/typings";
import {handleSwitchToGuided} from "../Freeform/presto-search-requests";
import {handleSwitchToFreeform} from "../Guided/presto-guided-search-requests";
import styles from "./index.module.css";


/**
 * Renders the SQL interface selector.
 *
 * @return
 */
const SqlInterfaceSelector = () => {
    const sqlInterface = usePrestoSearchState((state) => state.sqlInterface);
    const searchUiState = useSearchStore((state) => state.searchUiState);
    const disabled = searchUiState === SEARCH_UI_STATE.QUERY_ID_PENDING ||
        searchUiState === SEARCH_UI_STATE.QUERYING;

    const handleChange = useCallback((value: PRESTO_SQL_INTERFACE) => {
        if (value === sqlInterface) {
            return;
        }

        if (value === PRESTO_SQL_INTERFACE.GUIDED) {
            handleSwitchToGuided();

            return;
        }

        handleSwitchToFreeform();
    }, [sqlInterface]);


    return (
        <div className={styles["sqlInterfaceButton"]}>
            <Tabs
                activeKey={sqlInterface}
                onChange={(value) => handleChange(value as PRESTO_SQL_INTERFACE)}
                size="small"
                items={[
                    {
                        key: PRESTO_SQL_INTERFACE.GUIDED,
                        label: (
                            <span className={styles["tabLabel"]}>
                                <AppstoreOutlined/>
                                Guided
                            </span>
                        ),
                        disabled,
                    },
                    {
                        key: PRESTO_SQL_INTERFACE.FREEFORM,
                        label: (
                            <span className={styles["tabLabel"]}>
                                <EditOutlined/>
                                Manual
                            </span>
                        ),
                        disabled,
                    },
                ]}
            />
        </div>
    );
};

export default SqlInterfaceSelector;
