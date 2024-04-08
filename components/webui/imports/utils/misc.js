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

/**
 * Removes wrapping quotes from the given string, if it's quoted, and unescapes quotes from within
 * the quoted string.
 * NOTE: This method does *not* unescape non-quote characters, unlike most methods which handle
 * unescaping quotes.
 * @param str
 * @param quoteChar
 * @param escapeChar
 * @return The processed string
 * @throws Error if the quoted string has a quote within it (rather than at its ends) or it's
 * missing one of it's begin/end quotes.
 */
const unquoteString = (str, quoteChar, escapeChar) => {
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

    if (positionOfCharsToRemove.length === 0) {
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

    const processedChars = chars.filter((c, i) => false === positionOfCharsToRemove.includes(i));
    return processedChars.join("");
}

export {computeHumanSize, sleep, unquoteString};
