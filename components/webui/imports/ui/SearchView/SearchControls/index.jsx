import {
    useEffect,
    useRef,
    useState,
} from "react";
import Button from "react-bootstrap/Button";
import Form from "react-bootstrap/Form";
import InputGroup from "react-bootstrap/InputGroup";

import {
    faBars,
    faSearch,
    faTimes,
    faTrash,
} from "@fortawesome/free-solid-svg-icons";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";

import {CLP_STORAGE_ENGINES} from "/imports/api/constants";
import {
    isOperationInProgress,
    isSearchSignalReq,
    SEARCH_SIGNAL,
} from "/imports/api/search/constants";

import {LOCAL_STORAGE_KEYS} from "../../constants";
import SearchControlsFilterDrawer from "./SearchControlsFilterDrawer";


/**
 * Renders the search controls including query input, filter drawer toggle, and operation buttons
 * like submit, clear, and cancel. It also manages the state of the drawer.
 *
 * @param {object} props
 * @param {boolean} props.ignoreCase
 * @param {Function} props.onCancelOperation
 * @param {Function} props.onClearResults
 * @param {Function} props.onSubmitQuery
 * @param {string} props.queryString
 * @param {SearchResultsMetadata} props.resultsMetadata
 * @param {Function} props.setIgnoreCase
 * @param {Function} props.setQueryString
 * @param {Function} props.setTimeRange
 * @param {TimeRange} props.timeRange
 * @return {React.ReactElement}
 */
const SearchControls = ({
    ignoreCase,
    onCancelOperation,
    onClearResults,
    onSubmitQuery,
    queryString,
    resultsMetadata,
    setIgnoreCase,
    setQueryString,
    setTimeRange,
    timeRange,
}) => {
    const [drawerOpen, setDrawerOpen] = useState(
        "true" === localStorage.getItem(LOCAL_STORAGE_KEYS.SEARCH_CONTROLS_VISIBLE)
    );
    const inputRef = useRef(null);

    const isInputDisabled = isOperationInProgress(resultsMetadata.lastSignal);

    useEffect(() => {
        if (false === isInputDisabled) {
            inputRef.current?.focus();
        }
    }, [isInputDisabled]);

    useEffect(() => {
        localStorage.setItem(LOCAL_STORAGE_KEYS.SEARCH_CONTROLS_VISIBLE, drawerOpen.toString());
    }, [drawerOpen]);

    const queryChangeHandler = (e) => {
        setQueryString(e.target.value);
    };

    const handleDrawerToggleClick = () => {
        setDrawerOpen(!drawerOpen);
    };

    const handleQuerySubmission = (e) => {
        e.preventDefault();

        onSubmitQuery();
    };

    const queryPlaceholderText =
        CLP_STORAGE_ENGINES.CLP === Meteor.settings.public.ClpStorageEngine ?
            "Enter a wildcard query..." :
            "Enter a KQL query...";

    return (
        <>
            <Form onSubmit={handleQuerySubmission}>
                <Form.Group className={"mb-0 border-bottom"}>
                    <InputGroup>
                        <Button
                            active={false}
                            className={"border-top-0 border-bottom-0 rounded-0"}
                            variant={"secondary"}
                            onClick={handleDrawerToggleClick}
                        >
                            <FontAwesomeIcon icon={faBars}/>
                        </Button>
                        <Form.Control
                            autoFocus={true}
                            className={"border-top-0 border-bottom-0"}
                            disabled={isInputDisabled}
                            placeholder={queryPlaceholderText}
                            ref={inputRef}
                            type={"text"}
                            value={queryString}
                            onChange={queryChangeHandler}/>
                        {(SEARCH_SIGNAL.RESP_DONE === resultsMetadata.lastSignal) &&
                            <Button
                                className={"border-top-0 border-bottom-0 rounded-0"}
                                disabled={true === isSearchSignalReq(resultsMetadata.lastSignal)}
                                title={"Clear Results"}
                                variant={"info"}
                                onClick={onClearResults}
                            >
                                <FontAwesomeIcon
                                    fixedWidth={true}
                                    icon={faTrash}/>
                            </Button>}
                        {(SEARCH_SIGNAL.RESP_QUERYING === resultsMetadata.lastSignal) ?
                            <Button
                                className={"border-top-0 border-bottom-0 rounded-0"}
                                variant={"danger"}
                                disabled={SEARCH_SIGNAL.REQ_CANCELLING ===
                                    resultsMetadata.lastSignal}
                                onClick={onCancelOperation}
                            >
                                <FontAwesomeIcon
                                    fixedWidth={true}
                                    icon={faTimes}/>
                            </Button> :
                            <Button
                                className={"border-top-0 border-bottom-0 rounded-0"}
                                disabled={isInputDisabled || "" === queryString}
                                type={"submit"}
                                variant={"primary"}
                            >
                                <FontAwesomeIcon
                                    fixedWidth={true}
                                    icon={faSearch}/>
                            </Button>}
                    </InputGroup>
                </Form.Group>
            </Form>

            {drawerOpen && <SearchControlsFilterDrawer
                ignoreCase={ignoreCase}
                setIgnoreCase={setIgnoreCase}
                setTimeRange={setTimeRange}
                timeRange={timeRange}/>}
        </>
    );
};

export default SearchControls;
