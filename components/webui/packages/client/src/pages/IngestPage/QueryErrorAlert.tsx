import {
    Alert,
    Button,
} from "antd";


interface QueryErrorAlertProps {
    error: Error | null;
    hasPreviousData: boolean;
    onRetry: () => void;
}

/**
 * Shows a failed dashboard request and offers a retry.
 *
 * @param props
 * @param props.error
 * @param props.hasPreviousData
 * @param props.onRetry
 * @return
 */
const QueryErrorAlert = ({error, hasPreviousData, onRetry}: QueryErrorAlertProps) => (
    <Alert
        description={error?.message ?? "Request failed."}
        style={{gridColumn: "1 / -1"}}
        action={
            <Button
                size={"small"}
                onClick={onRetry}
            >
                Retry
            </Button>
        }
        title={hasPreviousData ?
            "Showing previously loaded data" :
            "Unable to load data"}
        type={hasPreviousData ?
            "warning" :
            "error"}/>
);

export default QueryErrorAlert;
