import {Meteor} from "meteor/meteor";
import {v4 as uuidv4} from "uuid";

// TODO: implement a full-fledged registration sys
const LOCAL_STORAGE_KEY_USERNAME = 'username';
const DUMMY_PASSWORD = 'DummyPassword';

let LoginRetryCount = 0;
const CONST_MAX_LOGIN_RETRY = 3;

export const registerAndLoginWithUsername = async (username) => {
    try {
        await new Promise((resolve, reject) => {
            Meteor.call('user.create', {username, password: DUMMY_PASSWORD}, (error) => {
                if (error) {
                    console.log('create user error', error);
                    reject(error);
                } else {
                    localStorage.setItem(LOCAL_STORAGE_KEY_USERNAME, username);
                    resolve(true);
                }
            });
        });

        return await loginWithUsername(username);
    } catch (error) {
        return false;
    }
};

export const loginWithUsername = async (username) => {
    try {
        return await new Promise((resolve, reject) => {
            Meteor.loginWithPassword(username, DUMMY_PASSWORD, (error) => {
                if (!error) {
                    resolve(true);
                } else {
                    console.log('login error', error, 'LOGIN_RETRY_COUNT:', LoginRetryCount);
                    if (LoginRetryCount < CONST_MAX_LOGIN_RETRY) {
                        LoginRetryCount++;
                        resolve(registerAndLoginWithUsername(username));
                    } else {
                        reject(error);
                    }
                }
            });
        });
    } catch (error) {
        return false;
    }
};

export const login = async () => {
    let username = localStorage.getItem(LOCAL_STORAGE_KEY_USERNAME);
    let result;

    if (username === null) {
        username = uuidv4();
        result = await registerAndLoginWithUsername(username);
    } else {
        result = await loginWithUsername(username);
    }

    return result
}
