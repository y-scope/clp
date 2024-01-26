import {faCog} from "@fortawesome/free-solid-svg-icons";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";
import React from "react";
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

export const SearchResultsHeader = ({
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

    let numResultsText = `(Job ID ${jobId}) `;
    if (0 === numResultsOnServer) {
        numResultsText += SearchSignal.RSP_DONE !== resultsMetadata["lastSignal"] ?
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
                <Col className={"mr-auto"}><span
                    className={"search-results-count"}>{numResultsText}</span></Col>
                <Col className={"pr-0"} xs={"auto"}>
                    <OverlayTrigger
                        placement={"left"}
                        trigger={"click"}
                        overlay={<Popover id={"searchResultsDisplaySettings"}
                                          className={"search-results-display-settings-container"}>
                            <Form onSubmit={handleMaxLinesPerResultSubmission}>
                                <InputGroup className={"search-results-display-settings"}>
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
                        <Button type={"button"} variant={"light"}
                                title={"Display Settings"}><FontAwesomeIcon
                            icon={faCog}/></Button>
                    </OverlayTrigger>
                </Col>
            </Row>
        </Container>
    </>);
};