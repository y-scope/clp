import {CssVarsProvider} from "@mui/joy";
import {useState} from "react";
import axios from "axios";

import {LOCAL_STORAGE_KEY} from "./typings/config";
import QueryStatus from "./ui/QueryStatus";


/**
 * Renders the main application.
 *
 * @return
 */
const App = () => {
    const [output, setOutput] = useState("");
    const [queryString, setQueryString] = useState("test query");
    const [timestampBegin, setTimestampBegin] = useState(0);
    const [timestampEnd, setTimestampEnd] = useState(Date.now());
    const [ignoreCase, setIgnoreCase] = useState(true);
    const [timeRangeBucketSizeMillis, setTimeRangeBucketSizeMillis] = useState(60000);
    const [searchJobId, setSearchJobId] = useState(1);
    const [aggregationJobId, setAggregationJobId] = useState(2);

    const sendRequest = async (url: string, method: string, body: object) => {
        try {
            const response = await axios({
                url,
                method,
                data: body,
                headers: {"Content-Type": "application/json"},
            });
            setOutput(JSON.stringify(response.data, null, 2));
        } catch (error: unknown) {
            if (axios.isAxiosError(error)) {
                setOutput(`Error: ${error.response?.data?.message || error.message}`);
            } else {
                setOutput(`Error: ${String(error)}`);
            }
        }
    };

    return (
        <CssVarsProvider modeStorageKey={LOCAL_STORAGE_KEY.THEME}>
            <QueryStatus/>
            <div style={{padding: "20px"}}>
                <h1>Search API Test</h1>
                <div>
                    <label>
                        Query String:
                        <input
                            type="text"
                            value={queryString}
                            onChange={(e) => setQueryString(e.target.value)}
                        />
                    </label>
                    <label>
                        Timestamp Begin:
                        <input
                            type="number"
                            value={timestampBegin}
                            onChange={(e) => setTimestampBegin(Number(e.target.value))}
                        />
                    </label>
                    <label>
                        Timestamp End:
                        <input
                            type="number"
                            value={timestampEnd}
                            onChange={(e) => setTimestampEnd(Number(e.target.value))}
                        />
                    </label>
                    <label>
                        Ignore Case:
                        <input
                            type="checkbox"
                            checked={ignoreCase}
                            onChange={(e) => setIgnoreCase(e.target.checked)}
                        />
                    </label>
                    <label>
                        Time Range Bucket Size (ms):
                        <input
                            type="number"
                            value={timeRangeBucketSizeMillis}
                            onChange={(e) => setTimeRangeBucketSizeMillis(Number(e.target.value))}
                        />
                    </label>
                    <label>
                        Search Job ID:
                        <input
                            type="number"
                            value={searchJobId}
                            onChange={(e) => setSearchJobId(Number(e.target.value))}
                        />
                    </label>
                    <label>
                        Aggregation Job ID:
                        <input
                            type="number"
                            value={aggregationJobId}
                            onChange={(e) => setAggregationJobId(Number(e.target.value))}
                        />
                    </label>
                </div>
                <button
                    onClick={() =>
                        sendRequest("/api/search/search", "POST", {
                            queryString,
                            timestampBegin,
                            timestampEnd,
                            ignoreCase,
                            timeRangeBucketSizeMillis,
                        })
                    }
                >
                    Submit Search
                </button>
                <button
                    onClick={() =>
                        sendRequest("/api/search/search/results", "DELETE", {
                            searchJobId,
                            aggregationJobId,
                        })
                    }
                >
                    Clear Results
                </button>
                <button
                    onClick={() =>
                        sendRequest("/api/search/search/cancel", "POST", {
                            searchJobId,
                            aggregationJobId,
                        })
                    }
                >
                    Cancel Search
                </button>
                <pre style={{marginTop: "20px", background: "#f4f4f4", padding: "10px"}}>
                    {output}
                </pre>
            </div>
        </CssVarsProvider>
    );
};

export default App;
