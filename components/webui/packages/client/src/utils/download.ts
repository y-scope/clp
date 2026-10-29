/**
 * Defer revocation by this amount because revoking the blob URL immediately after `click()`
 * silently aborts downloads on some browsers (e.g., Firefox).
 */
const REVOCATION_DELAY_MILLIS = 60_000;

/**
 * Triggers a plain-text file download in the browser by creating a temporary
 * Blob URL and clicking a hidden anchor element.
 *
 * @param lines Iterable of strings to write into the file.
 * @param filename Name for the downloaded file.
 */
const downloadTextFile = (lines: Iterable<string>, filename: string): void => {
    const blob = new Blob([...lines], {type: "text/plain"});
    const url = URL.createObjectURL(blob);
    const anchor = document.createElement("a");
    anchor.href = url;
    anchor.download = filename;
    anchor.click();
    setTimeout(() => {
        URL.revokeObjectURL(url);
    }, REVOCATION_DELAY_MILLIS);
};


export {downloadTextFile};
