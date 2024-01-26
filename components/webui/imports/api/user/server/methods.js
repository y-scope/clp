import {Meteor} from "meteor/meteor";
import {Accounts} from 'meteor/accounts-base';

Meteor.methods({
    /**
     * Creates a user account with a provided username and password.
     *
     * @param {string} username for the new user
     * @param {string} password for the new user
     */
    'user.create'({username, password}) {
        Accounts.createUser({username, password});
    }
})
