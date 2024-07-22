import {
    useEffect,
    useRef,
} from "react";

import axios from "axios";


/**
 * Submits a job to extract an IR
 *
 * @param {number|string} origFileId The ID of the original file to extract IR from
 * @param {number} logEventIx The index of the log event
 */
const submitExtractIrJob = async (origFileId, logEventIx) => {
    const {data} = await axios.post("/query/extract-ir", {
        msg_ix: logEventIx,
        orig_file_id: origFileId,
    });

    window.location = `/log-viewer/index.html?filePath=http://${window.location.host}/ir/${data.path}`;
};

/**
 * Renders the main application.
 *
 * @return {JSX.Element}
 */
const App = () => {
    const isFirstRun = useRef(true);

    useEffect(() => {
        if (false === isFirstRun.current) {
            return;
        }
        isFirstRun.current = false;

        const searchParams = new URLSearchParams(window.location.search);
        const origFileId = searchParams.get("origFileId");
        const logEventIx = searchParams.get("logEventIx");
        if (null === origFileId || null === logEventIx) {
            console.error("Either origFileId or logEventIx is missing from the URL parameters");
        }
        submitExtractIrJob(origFileId, logEventIx).then();
    }, []);

    return (
        <h1>Hello world!</h1>
    );
};

export default App;
