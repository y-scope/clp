/* eslint-disable jsdoc/check-param-names, jsdoc/require-description, jsdoc/require-returns */
/* eslint-disable no-use-before-define */

import {Progress as ProgressPrimitive} from "@base-ui/react/progress";

import {cn} from "@/lib/utils";


/**
 *
 * @param root0
 * @param root0.className
 * @param root0.children
 * @param root0.value
 */
const Progress = ({
    className,
    children,
    value,
    ...props
}: ProgressPrimitive.Root.Props) => {
    return (
        <ProgressPrimitive.Root
            className={cn("flex flex-wrap gap-3", className)}
            data-slot={"progress"}
            value={value}
            {...props}
        >
            {children}
            <ProgressTrack>
                <ProgressIndicator/>
            </ProgressTrack>
        </ProgressPrimitive.Root>
    );
};

/**
 *
 * @param root0
 * @param root0.className
 */
const ProgressTrack = ({className, ...props}: ProgressPrimitive.Track.Props) => {
    return (
        <ProgressPrimitive.Track
            data-slot={"progress-track"}
            className={cn(
                "relative flex h-1 w-full items-center overflow-x-hidden rounded-full bg-muted",
                className
            )}
            {...props}/>
    );
};

/**
 *
 * @param root0
 * @param root0.className
 */
const ProgressIndicator = ({
    className,
    ...props
}: ProgressPrimitive.Indicator.Props) => {
    return (
        <ProgressPrimitive.Indicator
            className={cn("h-full bg-primary transition-all", className)}
            data-slot={"progress-indicator"}
            {...props}/>
    );
};

/**
 *
 * @param root0
 * @param root0.className
 */
const ProgressLabel = ({className, ...props}: ProgressPrimitive.Label.Props) => {
    return (
        <ProgressPrimitive.Label
            className={cn("text-sm font-medium", className)}
            data-slot={"progress-label"}
            {...props}/>
    );
};

/**
 *
 * @param root0
 * @param root0.className
 */
const ProgressValue = ({className, ...props}: ProgressPrimitive.Value.Props) => {
    return (
        <ProgressPrimitive.Value
            data-slot={"progress-value"}
            className={cn(
                "ml-auto text-sm text-muted-foreground tabular-nums",
                className
            )}
            {...props}/>
    );
};

export {
    Progress,
    ProgressIndicator,
    ProgressLabel,
    ProgressTrack,
    ProgressValue,
};
