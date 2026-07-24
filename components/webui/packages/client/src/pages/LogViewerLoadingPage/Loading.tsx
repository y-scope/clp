import React from "react";

import {Nullable} from "@webui/common/utility-types";

import {
    QUERY_LOADING_STATE,
    QUERY_LOADING_STATE_DESCRIPTIONS,
    QUERY_LOADING_STATE_VALUES,
} from "../../typings/query";

import {
    Alert,
    AlertDescription,
    AlertTitle,
} from "@/components/ui/alert";
import {Progress} from "@/components/ui/progress";
import {cn} from "@/lib/utils";


interface LoadingStepProps {
    description: string;
    isActive: boolean;
    isError: boolean;
    label: string;
    stepIndicatorText: number | string;
}

/**
 * Renders a step with a label and description.
 *
 * @param props
 * @param props.description
 * @param props.isActive
 * @param props.isError
 * @param props.label
 * @param props.stepIndicatorText
 * @return
 */
const LoadingStep = ({
    description,
    isActive,
    isError,
    label,
    stepIndicatorText,
}: LoadingStepProps) => {
    return (
        <li
            className={cn(
                "grid grid-cols-[2.5rem_minmax(0,1fr)] items-start gap-3.5",
                "text-muted-foreground",
                isError ?
                    "text-destructive" :
                    isActive && "text-foreground"
            )}
        >
            <div
                className={cn(
                    "flex size-10 items-center justify-center rounded-full border",
                    "border-border text-sm font-semibold",
                    isError ?
                        "border-destructive bg-destructive text-primary-foreground" :
                        isActive &&
                        "border-primary bg-primary text-primary-foreground"
                )}
            >
                {stepIndicatorText}
            </div>
            <div className={"min-w-0"}>
                <div className={"text-lg font-semibold"}>
                    {label}
                </div>
                <div className={"mt-1 text-sm text-muted-foreground"}>
                    {description}
                </div>
            </div>
        </li>
    );
};

interface LoadingProps {
    currentState: QUERY_LOADING_STATE;
    errorMsg: Nullable<string>;
}

/**
 * Displays status of a pending query job.
 *
 * @param props
 * @param props.currentState
 * @param props.errorMsg
 * @return
 */
const Loading = ({
    currentState,
    errorMsg,
}: LoadingProps) => {
    const steps: React.ReactNode[] = [];
    QUERY_LOADING_STATE_VALUES.forEach((state) => {
        const isActive = (currentState === state);
        const stateDescription = QUERY_LOADING_STATE_DESCRIPTIONS[state];
        steps.push(
            <LoadingStep
                description={stateDescription.description}
                isActive={isActive}
                isError={false}
                key={state}
                label={stateDescription.label}
                stepIndicatorText={state + 1}/>
        );
        if (isActive && null !== errorMsg) {
            steps.push(
                <LoadingStep
                    description={errorMsg}
                    isActive={isActive}
                    isError={true}
                    key={`${state}-error`}
                    label={"Error"}
                    stepIndicatorText={"X"}/>
            );
        }
    });

    return (
        <div
            className={"flex h-full flex-col items-center bg-background text-foreground"}
        >
            <Progress
                className={cn(
                    "w-full",
                    null !== errorMsg && "[--primary:var(--destructive)]"
                )}
                value={null === errorMsg ?
                    null :
                    100}/>
            <div
                className={
                    "flex w-[calc(100%-2rem)] max-w-lg flex-1 flex-col " +
                    "items-center justify-center gap-6"
                }
            >
                {null !== errorMsg && (
                    <Alert variant={"destructive"}>
                        <AlertTitle>
                            Error
                        </AlertTitle>
                        <AlertDescription>
                            {errorMsg}
                        </AlertDescription>
                    </Alert>
                )}
                <ol className={"flex w-full flex-col gap-8"}>
                    {steps}
                </ol>
            </div>
        </div>
    );
};

export default Loading;
