import { io, Socket } from "socket.io-client";

let sharedSocket: Socket | null = null;

// Singleton pattern to create a single socket connection for the entire application..
export function getSharedSocket(): Socket {
    if (!sharedSocket) {
        // eslint-disable-next-line no-warning-comments
        // TODO: Add support for user provided domain name (i.e. io("https://server-domain.com")).
        // Implementation could involve parsing server .env file and moving server .env to a
        // common folder.
        sharedSocket = io();
    }
    return sharedSocket;
}