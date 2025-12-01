import {ValueErrorType} from "@sinclair/typebox/errors";
import {Value} from "@sinclair/typebox/value";
import {
    DATASET_NAME_MAX_LEN,
    DatasetNameSchema,
} from "@webui/common/schemas/compression";


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

    if (false === Value.Check(DatasetNameSchema, datasetName)) {
        const [firstError] = [...Value.Errors(DatasetNameSchema, datasetName)];

        if (firstError) {
            // Check which validation failed using TypeBox error types
            if (ValueErrorType.StringMaxLength === firstError.type) {
                return "Dataset name can only be a maximum of " +
                    `${DATASET_NAME_MAX_LEN} characters long.`;
            }
            if (ValueErrorType.StringPattern === firstError.type) {
                return "Dataset name can only contain alphanumeric characters " +
                    "and underscores.";
            }
        }

        return firstError?.message ?? "Invalid dataset name.";
    }

    return null;
};


export {validateDatasetName};
