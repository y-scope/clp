// import { CssVarsProvider } from "@mui/joy";
// import { LOCAL_STORAGE_KEY } from "./typings/config";
// import QueryStatus from "./ui/QueryStatus";
import MongoReplicaCollection from "./mongoCDCLib/MongoReplicaCollection";
import {useTracker} from "./mongoCDCLib/useTracker";


const collection = new MongoReplicaCollection("results-1");

/**
 * Renders the main application.
 *
 * @return
 */
const App = () => {
    const results = useTracker(
        () => collection.find({}, {}),
        []
    );

    console.log("results len:", results.length);
    for (let i = 0; i < results.length; i++) {
        console.log(results[i]);
    }

    // const results = ['123'];
    // results = ["123"];

    return (
        <div>
            {results.map((r) => (
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
