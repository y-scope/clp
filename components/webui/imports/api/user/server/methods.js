import {Meteor} from "meteor/meteor";
import {Accounts} from 'meteor/accounts-base';

import {initializeWebsocket} from "../../search/server/query_handler_mediator";

Meteor.methods({
    'user.create'({username, password}) {
        Accounts.createUser({username, password});
    }
})

Accounts.onLogin(() => {
    initializeWebsocket(Accounts.userId());
})