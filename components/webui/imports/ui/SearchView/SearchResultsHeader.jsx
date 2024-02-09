import React from "react";

import {faCog} from "@fortawesome/free-solid-svg-icons";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";
import {
    Button,
    Col,
    Container,
    Form,
    InputGroup,
    OverlayTrigger,
    Popover,
    Row,
} from "react-bootstrap";
import {SearchSignal} from "../../api/search/constants";

import "./SearchResultsHeader.scss";


/**
 * Renders the header for the search results, which includes the job ID, the number of results
 * found, and a control for setting the maximum number of lines per search result.
 *
 * @param {number} jobId of the search job
 * @param {Object} resultsMetadata which includes last request / response signal
 * @param {number} numResultsOnServer of the search job
 * @param {number} maxLinesPerResult to display
 * @param {function} setMaxLinesPerResult callback to set setMaxLinesPerResult
 * @returns {JSX.Element}
 */
const SearchResultsHeader = ({
    jobId,
    resultsMetadata,
    numResultsOnServer,
    maxLinesPerResult,
    setMaxLinesPerResult,
}) => {
    const handleMaxLinesPerResultSubmission = (e) => {
        e.preventDefault();
        const value = parseInt(e.target.elements["maxLinesPerResult"].value);
        if (value > 0) {
            setMaxLinesPerResult(value);
        }
    };

    let numResultsText = `Job ID ${jobId}: `;
    if (0 === numResultsOnServer) {
        numResultsText += SearchSignal.RESP_DONE !== resultsMetadata["lastSignal"] ?
            "Query is running" :
            "No results found";
    } else if (1 === numResultsOnServer) {
        numResultsText += "1 result found";
    } else {
        numResultsText += `${numResultsOnServer} results found`;
    }

    return (<>
        <Container fluid={true}>
            <Row className={"search-results-title-bar"}>
                <Col>
                    <span className={"search-results-count"}>
                        Job ID {jobId} | Results count: {numResultsOnServer}
                    </span>
                </Col>
                <Col xs={"auto"}>
                    <OverlayTrigger
                        placement={"left"}
                        trigger={"click"}
                        overlay={<Popover id={"searchResultsDisplaySettings"}>
                            <Form onSubmit={handleMaxLinesPerResultSubmission}>
                                <InputGroup>
                                    <InputGroup.Text>Max lines per result</InputGroup.Text>
                                    <Form.Control
                                        id={"maxLinesPerResult"}
                                        name={"maxLinesPerResult"}
                                        type={"number"}
                                        defaultValue={maxLinesPerResult}
                                        min={1}
                                    />
                                </InputGroup>
                            </Form>
                        </Popover>}>
                        {(0 < numResultsOnServer) ? <Button type={"button"}
                                                            variant={"light"}
                                                            title={"Display Settings"}>
                            <FontAwesomeIcon icon={faCog}/>
                        </Button> : <></>}
                    </OverlayTrigger>
                </Col>
            </Row>
        </Container>
    </>);
};

export default SearchResultsHeader;
