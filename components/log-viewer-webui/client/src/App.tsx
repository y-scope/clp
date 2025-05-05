import { listItemButtonClasses } from "@mui/joy";
import MongoCollectionSocket from "./api/socket/MongoCollectionSocket";
import {useCursor} from "./api/socket/useCursor";
import {useState} from "react";



//const collection2 = new MongoCollectionSocket("stats");

//const collection2 = new MongoCollectionSocket("compression-jobs");

/**
 * Renders the main application.
 *
 * @return
 */
const SingleResultViewer = ({ collection }: { collection: MongoCollectionSocket }) => {
    const singleResult = useCursor(
        () => collection.find({}, { limit: 1 }),
        []
    );

    return (
        <div>
            {singleResult.map((r, index) => (
                <div key={index}>{JSON.stringify(r)}</div>
            ))}
        </div>
    );
};

/**
 * Renders the main application.
 *
 * @return
 */
const SingleResult2Viewer = ({ collection }: { collection: MongoCollectionSocket }) => {
    const singleResult = useCursor(
        () => collection.find({}, { limit: 5 }),
        []
    );

    return (
        <div>
            {singleResult.map((r, index) => (
                <div key={index}>{JSON.stringify(r)}</div>
            ))}
        </div>
    );
};
// Move this to top of app
const collection = new MongoCollectionSocket("compression-jobs");
const collection2 = new MongoCollectionSocket("stats");

const App = () => {

    const [show, setShow] = useState(true);

    return (
        <div>
            <div>
                <button onClick={() => setShow(!show)}>Toggle</button>
                {show && <SingleResultViewer collection={collection} />}
            </div>

            <p>------------------------</p>

            <div>
                <button onClick={() => setShow(!show)}>Toggle</button>
                {show && <SingleResult2Viewer collection={collection2} />}
            </div>


            <p>------------------------</p>
        </div>

    );
};

export default App;


//{results.map((r, index) => (
    //<div key={index}> {/* Add a unique key */}
   //     {JSON.stringify(r)}
   // </div>
//))}