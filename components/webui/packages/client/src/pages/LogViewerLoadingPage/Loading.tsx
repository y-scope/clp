import React from "react";

import {Nullable} from "@webui/common/utility-types";

import {
    QUERY_LOADING_STATE,
    QUERY_LOADING_STATE_DESCRIPTIONS,
    QUERY_LOADING_STATE_VALUES,
} from "../../typings/query";

import "./Loading.css";

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
    const isHighlighted = isActive || isError;

    return (
        <li
            className={cn(
                "loading-step",
                isHighlighted && "loading-step-active",
                isError && "loading-step-error"
            )}
        >
            <div className={"loading-step-indicator"}>
                {stepIndicatorText}
            </div>
            <div className={"loading-step-content"}>
                <div className={"loading-step-label"}>
                    {label}
                </div>
                <div className={"loading-step-description"}>
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
        <div className={"loading-sheet"}>
            <div className={"loading-progress-container"}>
                <Progress
                    className={cn(
                        "loading-progress",
                        null !== errorMsg && "loading-progress-error"
                    )}
                    value={null === errorMsg ?
                        null :
                        100}/>
            </div>
            <div className={"loading-stepper-container"}>
                {null !== errorMsg && (
                    <Alert
                        className={"loading-error-alert"}
                        variant={"destructive"}
                    >
                        <AlertTitle>
                            Error
                        </AlertTitle>
                        <AlertDescription>
                            {errorMsg}
                        </AlertDescription>
                    </Alert>
                )}
                <ol className={"loading-stepper"}>
                    {steps}
                </ol>
            </div>
        </div>
    );
};

export default Loading;
