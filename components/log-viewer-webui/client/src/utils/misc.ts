/**
 * Deselects all selections within the browser viewport.
 */
const deselectAll = () => {
    const selection = window.getSelection();
    if (null !== selection) {
        selection.removeAllRanges();
    }
};

export {deselectAll};
