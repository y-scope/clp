import {logger} from "/imports/utils/logger";
import {Accounts} from "meteor/accounts-base";
import {Meteor} from "meteor/meteor";


Meteor.methods({
    /**
     * Creates a user account with a provided username and password.
     *
     * @param {string} username for the new user
     * @param {string} password for the new user
     */
    "user.create"({
        username,
        password,
    }) {
        logger.info("user.create", `username=${username}`);
        Accounts.createUser({
            username,
            password,
        });
    },
});
