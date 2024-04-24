const MILLIS_PER_SECOND = 1000;
const BYTES_PER_KIBIBYTE = 1024;

/**
 * Creates a promise that resolves after a specified number of seconds.
 *
 * @param {number} seconds to wait before resolving the promise
 * @return {Promise<void>} that resolves after the specified delay
 */
const sleep = (seconds) => new Promise((resolve) => {
    setTimeout(resolve, seconds * MILLIS_PER_SECOND);
});

/**
 * Computes a human-readable representation of a size in bytes.
 *
 * @param {number} num
 * @return {string}
 */
const computeHumanSize = (num) => {
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

/**
 * Deselects all selections within the browser viewport.
 */
const deselectAll = () => {
    window.getSelection().removeAllRanges();
};


/**
 * Removes wrapping quotes from the given string, if it's quoted, and unescapes quotes from within
 * the quoted string.
 * NOTE: This method does *not* unescape non-quote characters, unlike most methods which handle
 * unescaping quotes.
 *
 * @param {string} str
 * @param {string} quoteChar
 * @param {string} escapeChar
 * @return {string} The processed string
 * @throws Error if the quoted string has a quote within it (rather than at its ends) or it's
 * missing one of it's begin/end quotes.
 */
// eslint-disable-next-line max-statements
const unquoteString = (
    str,
    quoteChar,
    escapeChar,
) => {
    if (0 === str.length) {
        return str;
    }

    // Determine the position of every character that we should remove from the processed string
    const positionOfCharsToRemove = [];
    const chars = Array.from(str);
    let isEscaped = false;
    for (let i = 0; i < chars.length; ++i) {
        const c = chars[i];
        if (isEscaped) {
            isEscaped = false;
            if (c === quoteChar) {
                // We only remove the escape characters that escape quotes
                positionOfCharsToRemove.push(i - 1);
            }
        } else if (c === escapeChar) {
            isEscaped = true;
        } else if (c === quoteChar) {
            positionOfCharsToRemove.push(i);
        }
    }

    if (0 === positionOfCharsToRemove.length) {
        return str;
    }

    // Ensure any unescaped quotes are only at the beginning and end of the string
    let foundBeginQuote = false;
    let foundEndQuote = false;
    positionOfCharsToRemove.forEach((pos) => {
        const char = chars[pos];
        if (quoteChar === char) {
            if (0 === pos) {
                foundBeginQuote = true;
            } else if (chars.length - 1 === pos) {
                foundEndQuote = true;
            } else {
                throw new Error(`Found unescaped quote character (${quoteChar}) within.`);
            }
        }
    });
    if (foundBeginQuote ^ foundEndQuote) {
        throw new Error("Begin/end quote is missing.");
    }

    const processedChars = chars.filter(
        (c, i) => (
            false === positionOfCharsToRemove.includes(i)
        )
    );

    return processedChars.join("");
};

export {
    computeHumanSize,
    deselectAll,
    sleep,
    unquoteString,
};
