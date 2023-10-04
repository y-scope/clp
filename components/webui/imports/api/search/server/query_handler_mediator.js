import {Meteor} from "meteor/meteor";

import WebSocket from "ws";

import {SearchState} from "../constants";

const ServerMessageType = {
    ERROR: 0,
    OPERATION_COMPLETE: 1,
    PREPARING_FOR_QUERY: 2,
    QUERY_STARTED: 3,
};

export const currentServerState = {state: null, errorMessage: ""};

class WebUiQueryHandlerWebsocket {
    constructor() {
        this.__websocket = null;
    }

    static __onOpen() {
        currentServerState.state = SearchState.READY;
    }

    __onClose() {
        currentServerState.state = SearchState.CONNECTING;
        setTimeout(() => {
            this.connect();
        }, 1000);
    }

    static __onError(e) {
        console.error(e);
    }

    __onMessage(e) {
        let message = JSON.parse(e.data);

        let messageType = message["type"];
        switch (messageType) {
            case ServerMessageType.OPERATION_COMPLETE:
                switch (currentServerState.state) {
                    case SearchState.QUERY_IN_PROGRESS:
                    case SearchState.CLEAR_RESULTS_IN_PROGRESS:
                        currentServerState.state = SearchState.READY;
                        break;
                    case SearchState.CONNECTING:
                    case SearchState.WAITING_FOR_QUERY_TO_VALIDATE:
                    case SearchState.WAITING_FOR_QUERY_TO_START:
                        // Should not be possible, so disconnect
                        console.error("Got OPERATION_COMPLETE while in impossible state.");
                        this.__websocket.close();
                        break;
                    case SearchState.READY:
                        // Do nothing
                        break;
                }
                break;
            case ServerMessageType.ERROR:
                switch (currentServerState.state) {
                    case SearchState.WAITING_FOR_QUERY_TO_VALIDATE:
                    case SearchState.WAITING_FOR_QUERY_TO_START:
                    case SearchState.QUERY_IN_PROGRESS:
                    case SearchState.CLEAR_RESULTS_IN_PROGRESS:
                        currentServerState.errorMessage = message["value"];
                        currentServerState.state = SearchState.READY;
                        break;
                    case SearchState.CONNECTING:
                        // Should not be possible, so disconnect
                        console.error("Got ERROR while in impossible state.");
                    // Fall through
                    case SearchState.READY:
                        // Something went wrong on the server, so disconnect to reset
                        this.__websocket.close();
                        break
                }
                break;
            case ServerMessageType.PREPARING_FOR_QUERY:
                if (SearchState.WAITING_FOR_QUERY_TO_VALIDATE === currentServerState.state) {
                    currentServerState.state = SearchState.WAITING_FOR_QUERY_TO_START;
                } else {
                    // Should not be possible, so disconnect
                    console.error("Got PREPARING_FOR_QUERY while in impossible state.");
                    this.__websocket.close();
                }
                break;
            case ServerMessageType.QUERY_STARTED:
                if (SearchState.WAITING_FOR_QUERY_TO_START === currentServerState.state) {
                    currentServerState.state = SearchState.QUERY_IN_PROGRESS;
                } else {
                    // Should not be possible, so disconnect
                    console.error("Got QUERY_STARTED while in impossible state.");
                    this.__websocket.close();
                }
                break;
        }
    }

    connect() {
        this.__websocket = new WebSocket(`ws://${Meteor.settings.private.QueryHandlerHost}:${Meteor.settings.private.QueryHandlerWebsocket}`);
        this.__websocket.onopen = WebUiQueryHandlerWebsocket.__onOpen;
        this.__websocket.onclose = () => {
            this.__onClose();
        };
        this.__websocket.onerror = WebUiQueryHandlerWebsocket.__onError;
        this.__websocket.onmessage = this.__onMessage;
    }

    send(message) {
        this.__websocket.send(message);
    }
}

export function initialize() {
    currentServerState.state = SearchState.CONNECTING;
    webuiQueryHandlerWebsocket.connect();
}

export const webuiQueryHandlerWebsocket = new WebUiQueryHandlerWebsocket();
