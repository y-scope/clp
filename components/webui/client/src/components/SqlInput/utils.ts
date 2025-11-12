/**
 * Escapes Markdown special characters in text for safe display in Monaco editor hover tooltips.
 *
 * @param text
 * @return Escaped text.
 */
const escapeHoverMarkdown = (text: string) => text
    .replace(/\\/g, "\\\\")
    .replace(/\*/g, "\\*")
    .replace(/#/g, "\\#")
    .replace(/\//g, "\\/")
    .replace(/\(/g, "\\(")
    .replace(/\)/g, "\\)")
    .replace(/\[/g, "\\[")
    .replace(/\]/g, "\\]")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/_/g, "\\_")
    .replace(/`/g, "\\`");

export {escapeHoverMarkdown};
