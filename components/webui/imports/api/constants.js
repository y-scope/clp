/**
 * Enum of the CLP package's possible storage engines. These must match the values in
 * clp_py_utils.clp_config.StorageEngine.
 *
 * @enum {string}
 */
const CLP_STORAGE_ENGINES = Object.freeze({
    CLP: "clp",
    CLP_S: "clp-s",
});

export {CLP_STORAGE_ENGINES};
