const MILLIS_PER_SECOND = 1000;

/**
 * Creates a promise that resolves after a specified number of seconds.
 *
 * @param {number} seconds Number of seconds to wait before resolving the promise.
 * @return {Promise<void>} A promise that resolves after the specified delay.
 */
const sleep = (seconds) => new Promise((resolve) => {
    setTimeout(resolve, seconds * MILLIS_PER_SECOND);
});

export {sleep};
