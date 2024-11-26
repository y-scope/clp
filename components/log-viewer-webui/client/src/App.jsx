import {CssVarsProvider} from "@mui/joy/styles/CssVarsProvider";

import LOCAL_STORAGE_KEY from "./typings/LOCAL_STORAGE_KEY.js";
import QueryStatus from "./ui/QueryStatus.jsx";


/**
 * Renders the main application.
 *
 * @return {JSX.Element}
 */
const App = () => {
    return (
        <CssVarsProvider modeStorageKey={LOCAL_STORAGE_KEY.THEME}>
            <QueryStatus/>
        </CssVarsProvider>
    );
};

export default App;
