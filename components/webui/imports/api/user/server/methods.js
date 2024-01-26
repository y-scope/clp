import {Meteor} from "meteor/meteor";
import {Accounts} from 'meteor/accounts-base';

Meteor.methods({
    'user.create'({username, password}) {
        Accounts.createUser({username, password});
    }
})
