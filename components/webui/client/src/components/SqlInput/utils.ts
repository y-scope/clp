/**
 * Escapes angle brackets in text for safe display in Monaco editor hover tooltips.
 * Monaco uses Markdown for hover messages, so raw `<` and `>` characters are
 * interpreted as HTML tags and must be escaped to display literals like `<EOF>`.
 *
 * @param text
 * @return Escaped text.
 */
const escapeHoverMarkdown = (text: string) => text.replace(/[<>]/g, (ch) => ("<" === ch ?
    "&lt;" :
    "&gt;"));

export {escapeHoverMarkdown};
