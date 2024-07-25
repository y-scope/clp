import {CssVarsProvider} from "@mui/joy/styles/CssVarsProvider";

import LOCAL_STORAGE_KEY from "./typings/LOCAL_STORAGE_KEY.js";
import Query from "./ui/QueryState.jsx";


/**
 * Renders the main application.
 *
 * @return {JSX.Element}
 */
const App = () => {
    return (
        <CssVarsProvider modeStorageKey={LOCAL_STORAGE_KEY.UI_THEME}>
            <Query/>
        </CssVarsProvider>
    );
};

export default App;
