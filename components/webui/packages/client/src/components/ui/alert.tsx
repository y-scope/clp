/* eslint-disable @stylistic/max-len, jsdoc/check-param-names, jsdoc/require-description, jsdoc/require-returns */

import * as React from "react";

import {
    cva,
    type VariantProps,
} from "class-variance-authority";

import {cn} from "@/lib/utils";


const alertVariants = cva(
    "group/alert relative grid w-full gap-0.5 rounded-lg border px-2.5 py-2 text-left text-sm has-data-[slot=alert-action]:relative has-data-[slot=alert-action]:pr-18 has-[>svg]:grid-cols-[auto_1fr] has-[>svg]:gap-x-2 *:[svg]:row-span-2 *:[svg]:translate-y-0.5 *:[svg]:text-current *:[svg:not([class*='size-'])]:size-4",
    {
        variants: {
            variant: {
                default: "bg-card text-card-foreground",
                destructive:
          "bg-card text-destructive *:data-[slot=alert-description]:text-destructive/90 *:[svg]:text-current",
            },
        },
        defaultVariants: {
            variant: "default",
        },
    }
);

/**
 *
 * @param root0
 * @param root0.className
 * @param root0.variant
 */
const Alert = ({
    className,
    variant,
    ...props
}: React.ComponentProps<"div"> & VariantProps<typeof alertVariants>) => {
    return (
        <div
            className={cn(alertVariants({variant}), className)}
            data-slot={"alert"}
            role={"alert"}
            {...props}/>
    );
};

/**
 *
 * @param root0
 * @param root0.className
 */
const AlertTitle = ({className, ...props}: React.ComponentProps<"div">) => {
    return (
        <div
            data-slot={"alert-title"}
            className={cn(
                "font-medium group-has-[>svg]/alert:col-start-2 [&_a]:underline [&_a]:underline-offset-3 [&_a]:hover:text-foreground",
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
const AlertDescription = ({
    className,
    ...props
}: React.ComponentProps<"div">) => {
    return (
        <div
            data-slot={"alert-description"}
            className={cn(
                "text-sm text-balance text-muted-foreground md:text-pretty [&_a]:underline [&_a]:underline-offset-3 [&_a]:hover:text-foreground [&_p:not(:last-child)]:mb-4",
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
const AlertAction = ({className, ...props}: React.ComponentProps<"div">) => {
    return (
        <div
            className={cn("absolute top-2 right-2", className)}
            data-slot={"alert-action"}
            {...props}/>
    );
};

export {
    Alert, AlertAction, AlertDescription, AlertTitle,
};
