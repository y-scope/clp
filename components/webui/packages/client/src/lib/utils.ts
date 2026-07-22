import {
    type ClassValue,
    clsx,
} from "clsx";
import {twMerge} from "tailwind-merge";


/**
 * Merges class names with Tailwind conflict resolution.
 *
 * @param inputs Class name inputs.
 * @return The merged class name string.
 */
const cn = (...inputs: ClassValue[]) => twMerge(clsx(inputs));

export {cn};
