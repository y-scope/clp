import {CssVarsProvider} from "@mui/joy/styles/CssVarsProvider";

import Query from "./ui/QueryState.jsx";


/**
 * Renders the main application.
 *
 * @return {JSX.Element}
 */
const App = () => {
    return (
        <CssVarsProvider modeStorageKey={"uiTheme"}>
            <Query/>
        </CssVarsProvider>
    );
};

export default App;
