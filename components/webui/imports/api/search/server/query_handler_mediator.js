import {Meteor} from "meteor/meteor";

import WebSocket from "ws";

import {SearchState} from "../constants";

const ServerMessageType = {
    ERROR: 0,
    OPERATION_COMPLETE: 1,
    PREPARING_FOR_QUERY: 2,
    QUERY_STARTED: 3,
};

export const currentServerStateList = []

class WebUiQueryHandlerWebsocket {
    constructor() {
        this.__websocket = null;
        this.__sessionId = null;
    }

    __onOpen() {
        currentServerStateList[this.__sessionId].state = SearchState.READY;
    }

    __onClose() {
        currentServerStateList[this.__sessionId].state = SearchState.CONNECTING;
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
                switch (currentServerStateList[this.__sessionId].state) {
                    case SearchState.QUERY_IN_PROGRESS:
                    case SearchState.CLEAR_RESULTS_IN_PROGRESS:
                        currentServerStateList[this.__sessionId].state = SearchState.READY;
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
                switch (currentServerStateList[this.__sessionId].state) {
                    case SearchState.WAITING_FOR_QUERY_TO_VALIDATE:
                    case SearchState.WAITING_FOR_QUERY_TO_START:
                    case SearchState.QUERY_IN_PROGRESS:
                    case SearchState.CLEAR_RESULTS_IN_PROGRESS:
                        currentServerStateList[this.__sessionId].errorMessage = message["value"];
                        currentServerStateList[this.__sessionId].state = SearchState.READY;
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
                if (SearchState.WAITING_FOR_QUERY_TO_VALIDATE === currentServerStateList[this.__sessionId].state) {
                    currentServerStateList[this.__sessionId].state = SearchState.WAITING_FOR_QUERY_TO_START;
                } else {
                    // Should not be possible, so disconnect
                    console.error("Got PREPARING_FOR_QUERY while in impossible state.");
                    this.__websocket.close();
                }
                break;
            case ServerMessageType.QUERY_STARTED:
                if (SearchState.WAITING_FOR_QUERY_TO_START === currentServerStateList[this.__sessionId].state) {
                    currentServerStateList[this.__sessionId].state = SearchState.QUERY_IN_PROGRESS;
                    currentServerStateList[this.__sessionId].jobID = message['value']['jobID']
                } else {
                    // Should not be possible, so disconnect
                    console.error("Got QUERY_STARTED while in impossible state.");
                    this.__websocket.close();
                }
                break;
        }
    }

    connect() {
        this.__websocket = new WebSocket(`ws://${Meteor.settings.private.QueryHandlerHost}:${Meteor.settings.private.QueryHandlerWebsocket}/${this.__sessionId}`);
        this.__websocket.onopen = () => {
            this.__onOpen();
        }
        this.__websocket.onclose = () => {
            this.__onClose();
        };
        this.__websocket.onerror = WebUiQueryHandlerWebsocket.__onError;
        this.__websocket.onmessage = (e) => {
            this.__onMessage(e);
        };
    }

    send(message) {
        this.__websocket.send(message);
    }
}

export function initCurrentServerState(sessionId) {
    if (currentServerStateList[sessionId] === undefined) {
        currentServerStateList[sessionId] = {state: null, errorMessage: "", jobID: null};
    }
}

export function initializeWebsocket(sessionId) {
    if (webuiQueryHandlerWebsocketList[sessionId] === undefined) {
        webuiQueryHandlerWebsocketList[sessionId] = new WebUiQueryHandlerWebsocket();
        webuiQueryHandlerWebsocketList[sessionId].__sessionId = sessionId
    }

    initCurrentServerState(sessionId);
    currentServerStateList[sessionId].state = SearchState.CONNECTING;
    webuiQueryHandlerWebsocketList[sessionId].connect();
}


export const webuiQueryHandlerWebsocketList = [];
