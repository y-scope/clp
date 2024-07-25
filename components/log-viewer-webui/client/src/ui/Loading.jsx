import {
    Box,
    LinearProgress,
    Sheet,
    Step,
    StepIndicator,
    Stepper,
    Typography,
} from "@mui/joy";

import "./Loading.css";


/**
 * Descriptions for query states.
 */
const QUERY_STATE_DESCRIPTIONS = Object.freeze([
    {
        label: "Submitting query Job",
        description: "Parsing arguments and submitting job to the server.",
    },
    {
        label: "Waiting for job to finish",
        description: "The job is running. Waiting for the job to finish.",
    },
    {
        label: "Loading Log Viewer",
        description: "The query has been completed and the results are being loaded.",
    },
]);


/**
 * Renders a step with a label and description.
 *
 * @param {object} props
 * @param {string} props.description
 * @param {boolean} props.isActive
 * @param {boolean} props.isError
 * @param {string} props.label
 * @param {number | string} props.stepNumber
 * @return {React.ReactElement}
 */
const LoadingStep = ({
    description,
    isActive,
    isError,
    label,
    stepNumber,
}) => {
    let color = "danger";
    if (false === isError) {
        color = isActive ?
            "primary" :
            "neutral";
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
                    {stepNumber}
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
    QUERY_STATE_DESCRIPTIONS.forEach((state, index) => {
        const isActive = (currentState === index);
        steps.push(
            <LoadingStep
                description={state.description}
                isActive={isActive}
                isError={false}
                key={index}
                label={state.label}
                stepNumber={index + 1}/>
        );
        if (isActive && null !== errorMsg) {
            steps.push(
                <LoadingStep
                    description={errorMsg}
                    isActive={isActive}
                    isError={true}
                    key={`${index}-error`}
                    label={"Error"}
                    stepNumber={"X"}/>
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
