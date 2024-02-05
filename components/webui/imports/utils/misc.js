/**
 * Creates a promise that resolves after a specified number of seconds.
 *
 * @param {number} seconds to wait before resolving the promise
 * @returns {Promise<void>} that resolves after the specified delay
 */
export const sleep = (seconds) => new Promise(r => setTimeout(r, seconds * 1000));
