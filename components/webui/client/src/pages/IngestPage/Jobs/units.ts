/* eslint-disable @stylistic/array-element-newline */
const SI_UNITS = ["B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB", "RB", "QB"];
const IEC_UNITS = ["B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB", "RiB", "QiB"];
/* eslint-enable @stylistic/array-element-newline */

/**
 * Formats the given size value using either SI (kB, MB, etc.) or IEC (KiB, MiB, etc.) units.
 *
 * @param value The input value in bytes.
 * @param useSiUnits Whether to use SI units.
 * @param numFractionalDigits Number of digits to keep after the decimal point.
 * @return The value formatted as "<value> <unit>".
 */
const formatSizeInBytes = (
    value: number,
    useSiUnits: boolean = true,
    numFractionalDigits: number = 1
): string => {
    const units = useSiUnits ?
        SI_UNITS :
        IEC_UNITS;
    const divisor = useSiUnits ?
        10 ** 3 :
        2 ** 10;

    const multiplier = 10 ** numFractionalDigits;
    let unitIdx = 0;
    while (unitIdx < units.length - 1) {
        const roundedValue = Math.round(Math.abs(value) * multiplier) / multiplier;
        if (roundedValue < divisor) {
            break;
        }
        value /= divisor;
        ++unitIdx;
    }

    return `${value.toFixed(numFractionalDigits)} ${units[unitIdx]}`;
};

export {formatSizeInBytes};
