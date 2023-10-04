import {Meteor} from "meteor/meteor";

import {SearchState} from "../constants";
import {currentServerState, webuiQueryHandlerWebsocket} from "./query_handler_mediator";
import {ClientMessageType} from "./constants";

Meteor.methods({
    "search.submitQuery"({pipelineString, timestampBegin, timestampEnd, pathRegex, matchCase}) {
        currentServerState.errorMessage = "";

        switch (currentServerState.state) {
            case SearchState.CONNECTING:
                throw new Error("Lost connection to query handler.");
            case SearchState.WAITING_FOR_QUERY_TO_VALIDATE:
            case SearchState.WAITING_FOR_QUERY_TO_START:
            case SearchState.QUERY_IN_PROGRESS:
            case SearchState.CLEAR_RESULTS_IN_PROGRESS: {
                // Cancel current operation
                let message = {
                    type: ClientMessageType.CANCEL_OPERATION,
                };
                webuiQueryHandlerWebsocket.send(JSON.stringify(message));
            }
            // Fall through
            case SearchState.READY:
                let query = {
                    pipeline_string: pipelineString,
                    timestamp_begin: timestampBegin,
                    timestamp_end: timestampEnd,
                    path_regex: pathRegex,
                    match_case: matchCase,
                }
                let message = {
                    type: ClientMessageType.QUERY,
                    query: query,
                };

                webuiQueryHandlerWebsocket.send(JSON.stringify(message));
                currentServerState.state = SearchState.WAITING_FOR_QUERY_TO_VALIDATE;
                break;
        }
    },

    "search.clearResults"() {
        currentServerState.errorMessage = "";

        switch (currentServerState.state) {
            case SearchState.CONNECTING:
                throw new Error("Lost connection to query handler.");
            case SearchState.WAITING_FOR_QUERY_TO_VALIDATE:
            case SearchState.WAITING_FOR_QUERY_TO_START:
            case SearchState.QUERY_IN_PROGRESS:
            case SearchState.CLEAR_RESULTS_IN_PROGRESS: {
                // Cancel current operation
                let message = {
                    type: ClientMessageType.CANCEL_OPERATION,
                };
                webuiQueryHandlerWebsocket.send(JSON.stringify(message));
            }
            // Fall through
            case SearchState.READY:
                let message = {
                    type: ClientMessageType.CLEAR_RESULTS,
                };

                webuiQueryHandlerWebsocket.send(JSON.stringify(message));
                currentServerState.state = SearchState.CLEAR_RESULTS_IN_PROGRESS;
                break;
        }
    },

    "search.cancelOperation"() {
        currentServerState.errorMessage = "";

        switch (currentServerState.state) {
            case SearchState.CONNECTING:
                throw new Error("Lost connection to query handler.");
            case SearchState.WAITING_FOR_QUERY_TO_VALIDATE:
            case SearchState.WAITING_FOR_QUERY_TO_START:
            case SearchState.QUERY_IN_PROGRESS:
            case SearchState.CLEAR_RESULTS_IN_PROGRESS: {
                // Cancel current operation
                let message = {
                    type: ClientMessageType.CANCEL_OPERATION,
                };
                webuiQueryHandlerWebsocket.send(JSON.stringify(message));
                currentServerState.state = SearchState.READY;
                break;
            }
            case SearchState.READY:
                // Do nothing
                break;
        }
    },

    "search.updateTimelineRange"({timeRange}) {
        currentServerState.errorMessage = "";

        switch (currentServerState.state) {
            case SearchState.CONNECTING:
                throw new Error("Lost connection to query handler.");
            case SearchState.WAITING_FOR_QUERY_TO_VALIDATE:
            case SearchState.WAITING_FOR_QUERY_TO_START:
            case SearchState.CLEAR_RESULTS_IN_PROGRESS: {
                // Results don't exist or are being cleared, so ignore this request
                break;
            }
            case SearchState.READY:
            case SearchState.QUERY_IN_PROGRESS:
                let message = {
                    type: ClientMessageType.UPDATE_TIMELINE_RANGE,
                    time_range: timeRange
                };

                webuiQueryHandlerWebsocket.send(JSON.stringify(message));
                break;
        }
    },
});
