import {SqlTableSuffix} from "../../../config/sql-table-suffix";
import {settings} from "../../../settings";


/**
 * MySQL's maximum table name length.
 * Matches constants in `clp/components/clp-py-utils/clp_py_utils/clp_metadata_db_utils.py`.
 */
const MYSQL_TABLE_NAME_MAX_LEN = 64;

/**
 * Maximum length among all table suffixes.
 */
const TABLE_SUFFIX_MAX_LEN = Math.max(
    ...Object.values(SqlTableSuffix).map((suffix) => suffix.length)
);

/**
 * Calculates the maximum allowed dataset name length.
 *
 * @return The maximum allowed length for dataset names.
 */
const calculateDatasetNameMaxLength = (): number => {
    const clpTablePrefixLength = settings.SqlDbClpTablePrefix.length;

    // 1 For the separator between the dataset name and the table suffix
    return (
        MYSQL_TABLE_NAME_MAX_LEN -
        clpTablePrefixLength -
        1 -
        TABLE_SUFFIX_MAX_LEN
    );
};

/**
 * Validates that the given dataset name abides by the following rules:
 * - Its length won't cause any metadata table names to exceed MySQL's max table name length.
 * - It only contains alphanumeric characters and underscores.
 *
 * @param datasetName The dataset name to validate.
 * @return An error message if invalid, or null if valid.
 */
const validateDatasetName = (datasetName: string): string | null => {
    // Empty is valid (will use default)
    if (!datasetName) {
        return null;
    }

    if (!(/^\w+$/).test(datasetName)) {
        return "Dataset name can only contain alphanumeric characters and underscores.";
    }

    const maxLength = calculateDatasetNameMaxLength();
    if (datasetName.length > maxLength) {
        return `Dataset name can only be a maximum of ${maxLength} characters long.`;
    }

    return null;
};


export {
    calculateDatasetNameMaxLength,
    MYSQL_TABLE_NAME_MAX_LEN,
    TABLE_SUFFIX_MAX_LEN,
    validateDatasetName,
};
