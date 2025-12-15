import {ValueErrorType} from "@sinclair/typebox/errors";
import {Value} from "@sinclair/typebox/value";
import {
    AbsolutePathSchema,
    DATASET_NAME_MAX_LEN,
    DatasetNameSchema,
} from "@webui/common/schemas/compression";
import {Nullable} from "@webui/common/utility-types";


/**
 * Validates that all non-empty lines in the input are absolute paths using AbsolutePathSchema.
 *
 * @param value The multiline string input containing paths.
 * @return An error message if validation fails, or null if validation passes.
 */
const validateAbsolutePaths = (value: string): Nullable<string> => {
    const lines = value.split("\n");
    for (const [index, line] of lines.entries()) {
        const trimmedLine = line.trim();
        if (0 === trimmedLine.length) {
            continue;
        }
        if (false === Value.Check(AbsolutePathSchema, trimmedLine)) {
            return `Line ${index + 1}: Path must be absolute (start with "/").`;
        }
    }

    return null;
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


export {
    validateAbsolutePaths,
    validateDatasetName,
};
