import {CssVarsProvider} from "@mui/joy";

import LOCAL_STORAGE_KEY from "./typings/LOCAL_STORAGE_KEY";
import QueryStatus from "./ui/QueryStatus";


/**
 * Renders the main application.
 *
 * @return
 */
const App = () => {
    return (
        <CssVarsProvider modeStorageKey={LOCAL_STORAGE_KEY.THEME}>
            <QueryStatus/>
        </CssVarsProvider>
    );
};

export default App;
