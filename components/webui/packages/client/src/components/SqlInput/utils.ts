/**
 * Escapes Markdown special characters in text for safe display in Monaco editor hover tooltips.
 *
 * @param text
 * @return Escaped text.
 */
const escapeHoverMarkdown = (text: string) => {
    // eslint-disable-next-line no-useless-escape
    return text.replace(/([\\`*_\{\}\[\]<>\(\)#\+\-\.\!])/g, "\\$1");
};


export {escapeHoverMarkdown};
