/**
 * Returns a string, or an empty string if undefined.
 *
 * @param value
 * @return The normalized string.
 */
const safeString = (value: string | undefined): string => value || "";


export {safeString};
