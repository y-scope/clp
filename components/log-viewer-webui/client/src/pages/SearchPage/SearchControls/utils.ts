/**
 * Removes wrapping quotes from the given string, if it's quoted, and unescapes quotes from within
 * the quoted string.
 * NOTE: This method does *not* unescape non-quote characters, unlike most methods which handle
 * unescaping quotes.
 *
 * @param str
 * @param quoteChar
 * @param escapeChar
 * @return The processed string
 * @throws Error if the quoted string has a quote within it (rather than at its ends) or it's
 * missing one of it's begin/end quotes.
 */
// eslint-disable-next-line max-statements
const unquoteString = (
    str: string,
    quoteChar: string,
    escapeChar:string,
) => {
    if (0 === str.length) {
        return str;
    }

    // Determine the position of every character that we should remove from the processed string
    const positionOfCharsToRemove: number[] = [];
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
    for (const pos of positionOfCharsToRemove) {
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
    }
    if (foundBeginQuote !== foundEndQuote) {
        throw new Error("Begin/end quote is missing.");
    }

    const processedChars = chars.filter(
        (_, i) => (
            false === positionOfCharsToRemove.includes(i)
        )
    );

    return processedChars.join("");
};

export {unquoteString};
