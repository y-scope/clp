const MILLIS_PER_SECOND = 1000;
const BYTES_PER_KIBIBYTE = 1024;

/**
 * Creates a promise that resolves after a specified number of seconds.
 *
 * @param {number} seconds to wait before resolving the promise
 * @return {Promise<void>} that resolves after the specified delay
 */
const sleep = (seconds: number) => new Promise((resolve) => {
    setTimeout(resolve, seconds * MILLIS_PER_SECOND);
});

/**
 * Computes a human-readable representation of a size in bytes.
 *
 * @param {number} num
 * @return {string}
 */
const computeHumanSize = (num: number) => {
    // eslint-disable-next-line @stylistic/js/array-element-newline
    const siPrefixes = ["", "K", "M", "G", "T", "P", "E", "Z"];
    for (let i = 0; i < siPrefixes.length; ++i) {
        if (BYTES_PER_KIBIBYTE > Math.abs(num)) {
            return `${Math.round(num)} ${siPrefixes[i]}B`;
        }
        num /= BYTES_PER_KIBIBYTE;
    }

    return `${Math.round(num)} B`;
};


export {
    computeHumanSize,
    sleep,
};
