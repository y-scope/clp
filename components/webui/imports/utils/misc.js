/**
 * Creates a promise that resolves after a specified number of seconds.
 *
 * @param {number} seconds to wait before resolving the promise
 * @returns {Promise<void>} that resolves after the specified delay
 */
const sleep = (seconds) => new Promise(r => setTimeout(r, seconds * 1000));

/**
 * Computes a human-readable representation of a size in bytes.
 *
 * @param {number} num
 * @returns {string}
 */
const computeHumanSize = (num) => {
    const siPrefixes = ["", "K", "M", "G", "T", "P", "E", "Z"];
    for (let i = 0; i < siPrefixes.length; ++i) {
        if (Math.abs(num) < 1024.0) {
            return `${Math.round(num)} ${siPrefixes[i]}B`;
        }
        num /= 1024.0;
    }
    return `${Math.round(num)} B`;
};

export {computeHumanSize, sleep};
