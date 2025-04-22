/* eslint-disable react/jsx-key */

// import { CssVarsProvider } from "@mui/joy";
// import { LOCAL_STORAGE_KEY } from "./typings/config";
// import QueryStatus from "./ui/QueryStatus";
import MongoCollection from "./mongoCDCLib/MongoCollection";
import {useTracker} from "./mongoCDCLib/useTracker";


const collection = new MongoCollection("compression-jobs");

// const collection2 = new MongoReplicaCollection("compression-jobs");

/**
 * Renders the main application.
 *
 * @return
 */
const App = () => {
    const results = useTracker(
        () => collection.find({
            _id: {
                $gte: 1,
                $lte: 10,
            },
        }, {sort: {start_time: -1}}),
        []
    );

    const results2 = useTracker(
        () => collection.find({
            _id: {
                $gte: 1,
                $lte: 5,
            },
        }, {sort: {start_time: -1}}),
        []
    );

    /* for (let i = 0; i < results.length; i++) {
        console.log(results[i]);
    }

    for (let i = 0; i < results2.length; i++) {
        console.log(results2[i]);
    } */

    return (
        <div>
            {results.map((r) => (
                <div>
                    {JSON.stringify(r)}
                </div>
            ))}
            {results2.map((r) => (
                <div>
                    {JSON.stringify(r)}
                </div>
            ))}

        </div>

    // <CssVarsProvider modeStorageKey={LOCAL_STORAGE_KEY.THEME}>
    //     <QueryStatus/>
    // </CssVarsProvider>
    );
};

export default App;
