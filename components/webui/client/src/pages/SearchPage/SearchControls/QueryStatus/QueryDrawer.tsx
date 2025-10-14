import {Drawer, Typography, Alert} from "antd";
import usePrestoSearchState from "../../SearchState/Presto";
import useSearchStore from "../../SearchState";
import {SEARCH_UI_STATE} from "../../SearchState/typings";
import SqlEditor from "../../../../components/SqlEditor";

const {Text} = Typography;

const QueryDrawer = () => {
    const queryDrawerOpen = usePrestoSearchState((s) => s.queryDrawerOpen);
    const errorName = usePrestoSearchState((s) => s.errorName);
    const errorMsg = usePrestoSearchState((s) => s.errorMsg);
    const cachedGuidedSearchQueryString = usePrestoSearchState((s) => s.cachedGuidedSearchQueryString);
    const updateQueryDrawerOpen = usePrestoSearchState((s) => s.updateQueryDrawerOpen);
    const searchUiState = useSearchStore((s) => s.searchUiState);
    const searchJobId = useSearchStore((s) => s.searchJobId);

    return (
        <Drawer
            open={queryDrawerOpen}
            onClose={() => updateQueryDrawerOpen(false)}
            title={`Query Details \n ${searchJobId}`}
            placement="right"
        >
            <Text>
                {"Submitted Query: "}
            </Text>
            <div>
                <SqlEditor
                    value={cachedGuidedSearchQueryString}
                    height={"120px"}
                    disabled={false}
                    options={{
                        readOnly: true,
                        automaticLayout: true,
                        folding: false,
                        lineNumbers: "on",
                        minimap: {enabled: false},
                        overviewRulerBorder: false,
                        lineNumbersMinChars: 2,
                        padding: {
                            top: 8,
                            bottom: 8,
                        },
                        placeholder: "Enter your SQL query",
                        renderLineHighlight: "none",
                        scrollBeyondLastLine: false,
                        wordWrap: "off",
                    }}
                />
            </div>
            {searchUiState === SEARCH_UI_STATE.FAILED && (
                <Alert
                    type="error"
                    message={errorName}
                    description={errorMsg}
                    showIcon
                    style={{marginTop: 16}}
                />
            )}
        </Drawer>
    );
};

export default QueryDrawer;
