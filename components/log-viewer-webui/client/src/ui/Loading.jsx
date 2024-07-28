import {
    Box,
    LinearProgress,
    Sheet,
    Step,
    StepIndicator,
    Stepper,
    Typography,
} from "@mui/joy";

import {
    QUERY_LOADING_STATE_DESCRIPTIONS,
    QUERY_LOADING_STATES,
} from "../typings/query.js";

import "./Loading.css";


/**
 * Renders a step with a label and description.
 *
 * @param {object} props
 * @param {string} props.description
 * @param {boolean} props.isActive
 * @param {boolean} props.isError
 * @param {string} props.label
 * @param {number | string} props.stepIndicatorText
 * @return {React.ReactElement}
 */
const LoadingStep = ({
    description,
    isActive,
    isError,
    label,
    stepIndicatorText,
}) => {
    let color = isActive ?
        "primary" :
        "neutral";

    if (isError) {
        color = "danger";
    }

    return (
        <Step
            indicator={
                <StepIndicator
                    color={color}
                    variant={isActive ?
                        "solid" :
                        "outlined"}
                >
                    {stepIndicatorText}
                </StepIndicator>
            }
        >
            <Typography
                color={color}
                level={"title-lg"}
            >
                {label}
            </Typography>
            <Typography level={"body-sm"}>
                {description}
            </Typography>
        </Step>
    );
};

/**
 * Displays status of a pending query job.
 *
 * @param {object} props
 * @param {QueryLoadState} props.currentState
 * @param {string} props.errorMsg
 * @return {React.ReactElement}
 */
const Loading = ({currentState, errorMsg}) => {
    const steps = [];
    Object.values(QUERY_LOADING_STATES).forEach((state) => {
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
        <>
            <Sheet className={"loading-sheet"}>
                <Box className={"loading-progress-container"}>
                    <LinearProgress
                        determinate={null !== errorMsg}
                        color={null === errorMsg ?
                            "primary" :
                            "danger"}/>
                </Box>
                <Box className={"loading-stepper-container"}>
                    <Stepper
                        className={"loading-stepper"}
                        orientation={"vertical"}
                        size={"lg"}
                    >
                        {steps}
                    </Stepper>
                </Box>
            </Sheet>
        </>
    );
};

export default Loading;
