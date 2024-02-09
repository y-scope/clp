import {Meteor} from "meteor/meteor";
import {v4 as uuidv4} from "uuid";


// TODO: implement a full-fledged registration sys
const LOCAL_STORAGE_KEY_USERNAME = "username";
const DUMMY_PASSWORD = "DummyPassword";

let LoginRetryCount = 0;
const CONST_MAX_LOGIN_RETRY = 3;

/**
 * Registers a user with a provided username and then attempts to log in with that username.
 *
 * @param {string} username to register and log in with
 *
 * @returns {Promise<boolean>} true if the registration and login are successful
 *                             false if there's an error during registration or login
 */
const registerAndLoginWithUsername = async (username) => {
    return new Promise((resolve) => {
        Meteor.call("user.create", {
            username,
            password: DUMMY_PASSWORD,
        }, (error) => {
            if (error) {
                console.log("create user error", error);
                resolve(false);
            } else {
                localStorage.setItem(LOCAL_STORAGE_KEY_USERNAME, username);
                resolve(true);
            }
        });
    });
};

/**
 * Attempts to log in a user with the provided username using a dummy password.
 *
 * @param {string} username to register and log in with
 *
 * @returns {Promise<boolean>} true if the login is successful
 *                             false if there's an error during login or if the maximum login
 *                             retries are reached
 */
const loginWithUsername = (username) => {
    return new Promise((resolve) => {
        Meteor.loginWithPassword(username, DUMMY_PASSWORD, (error) => {
            if (!error) {
                resolve(true);
            } else {
                console.log("login error", error, "LOGIN_RETRY_COUNT:", LoginRetryCount);
                if (LoginRetryCount < CONST_MAX_LOGIN_RETRY) {
                    LoginRetryCount++;
                    resolve(registerAndLoginWithUsername(username));
                } else {
                    resolve(false);
                }
            }
        });
    });
};

/**
 * Attempts to log in a user using a stored username or register a new one if none is found.
 *
 * @returns {Promise<boolean>} true if the login is successful
 *                             false if there's an error during login or registration
 */
const login = async () => {
    let username = localStorage.getItem(LOCAL_STORAGE_KEY_USERNAME);
    let result;

    if (username === null) {
        username = uuidv4();
        result = await registerAndLoginWithUsername(username);
    } else {
        result = await loginWithUsername(username);
    }

    return result;
};

export {registerAndLoginWithUsername, loginWithUsername, login};
