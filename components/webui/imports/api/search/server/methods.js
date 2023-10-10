import {Meteor} from "meteor/meteor";

import {SearchState} from "../constants";
import {currentServerStateList, initCurrentServerState, webuiQueryHandlerWebsocketList} from "./query_handler_mediator";
import {ClientMessageType} from "./constants";

Meteor.methods({
    "search.submitQuery"({pipelineString, timestampBegin, timestampEnd, pathRegex, matchCase}) {
        initCurrentServerState(this.userId)
        currentServerStateList[this.userId].errorMessage = "";

        switch (currentServerStateList[this.userId].state) {
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
                webuiQueryHandlerWebsocketList[this.userId].send(JSON.stringify(message));
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

                webuiQueryHandlerWebsocketList[this.userId].send(JSON.stringify(message));
                currentServerStateList[this.userId].state = SearchState.WAITING_FOR_QUERY_TO_VALIDATE;
                break;
        }
    },

    "search.clearResults"() {
        initCurrentServerState(this.userId)
        currentServerStateList[this.userId].errorMessage = "";

        switch (currentServerStateList[this.userId].state) {
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
                webuiQueryHandlerWebsocketList[this.userId].send(JSON.stringify(message));
            }
            // Fall through
            case SearchState.READY:
                let message = {
                    type: ClientMessageType.CLEAR_RESULTS,
                };

                webuiQueryHandlerWebsocketList[this.userId].send(JSON.stringify(message));
                currentServerStateList[this.userId].state = SearchState.CLEAR_RESULTS_IN_PROGRESS;
                break;
        }
    },

    "search.cancelOperation"() {
        initCurrentServerState(this.userId)
        currentServerStateList[this.userId].errorMessage = "";

        switch (currentServerStateList[this.userId].state) {
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
                webuiQueryHandlerWebsocketList[this.userId].send(JSON.stringify(message));
                currentServerStateList[this.userId].state = SearchState.READY;
                break;
            }
            case SearchState.READY:
                // Do nothing
                break;
        }
    },

    "search.updateTimelineRange"({timeRange}) {
        initCurrentServerState(this.userId)
        currentServerStateList[this.userId].errorMessage = "";

        switch (currentServerStateList[this.userId].state) {
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

                webuiQueryHandlerWebsocketList[this.userId].send(JSON.stringify(message));
                break;
        }
    },
});
