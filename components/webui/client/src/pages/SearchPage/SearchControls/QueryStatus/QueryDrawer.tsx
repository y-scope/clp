import {
    Alert,
    Drawer,
    theme,
    Typography,
} from "antd";

import SqlEditor from "../../../../components/SqlEditor";
import useSearchStore from "../../SearchState";
import usePrestoSearchState from "../../SearchState/Presto";
import {SEARCH_UI_STATE} from "../../SearchState/typings";
import styles from "./QueryDrawer.module.css";


const {Text} = Typography;

/**
 * Renders the query details drawer, showing the modified SQL query and error (if any).
 *
 * @return
 */
const QueryDrawer = () => {
    const {token} = theme.useToken();
    const queryDrawerOpen = usePrestoSearchState((s) => s.queryDrawerOpen);
    const errorName = usePrestoSearchState((s) => s.errorName);
    const errorMsg = usePrestoSearchState((s) => s.errorMsg);
    const cachedGuidedSearchQueryString =
        usePrestoSearchState((s) => s.cachedGuidedSearchQueryString);
    const updateQueryDrawerOpen = usePrestoSearchState((s) => s.updateQueryDrawerOpen);
    const searchUiState = useSearchStore((s) => s.searchUiState);
    const searchJobId = useSearchStore((s) => s.searchJobId);

    return (
        <Drawer
            open={queryDrawerOpen}
            placement={"right"}
            title={`Query Details \n ${searchJobId ?? ""}`}
            styles={{
                body: {background: token.colorBgLayout},
                header: {
                    background: token.colorBgContainer,
                },
            }}
            onClose={() => {
                updateQueryDrawerOpen(false);
            }}
        >
            <div className={styles["container"]}>
                <Text strong={true}>
                    {"Modified SQL Query: "}
                </Text>
                <div>
                    <SqlEditor
                        disabled={false}
                        height={"120px"}
                        value={cachedGuidedSearchQueryString}
                        options={{
                            automaticLayout: true,
                            folding: false,
                            lineNumbers: "on",
                            lineNumbersMinChars: 2,
                            minimap: {enabled: false},
                            overviewRulerBorder: false,
                            padding: {
                                top: 8,
                                bottom: 8,
                            },
                            readOnly: true,
                            renderLineHighlight: "none",
                            scrollBeyondLastLine: false,
                            wordWrap: "on",
                        }}/>
                </div>
                {searchUiState === SEARCH_UI_STATE.FAILED && (
                    <Alert
                        description={errorMsg}
                        message={errorName}
                        showIcon={true}
                        type={"error"}/>
                )}
            </div>
        </Drawer>
    );
};

export default QueryDrawer;
