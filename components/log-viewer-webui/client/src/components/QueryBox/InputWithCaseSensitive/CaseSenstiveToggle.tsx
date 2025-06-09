import {useCallback} from "react";

import {
    Button,
    Tooltip,
    Typography,
} from "antd";


interface CaseSensitiveToggleProps {
    disabled?: boolean | undefined;
    isCaseSensitive: boolean;
    onCaseSensitiveChange: (newValue: boolean) => void;

}

/**
 * A toggle button component that switches between case-sensitive and case-insensitive
 * modes when clicked.
 *
 * @param props
 * @param props.disabled
 * @param props.isCaseSensitive
 * @param props.onCaseSensitiveChange
 * @return
 */
const CaseSensitiveToggle = ({
    disabled,
    isCaseSensitive,
    onCaseSensitiveChange,
}: CaseSensitiveToggleProps) => {
    const isDisabled = disabled ?? false;

    const handleButtonClick = useCallback(() => {
        if (!isDisabled) {
            onCaseSensitiveChange(!isCaseSensitive);
        }
    }, [
        isDisabled,
        isCaseSensitive,
        onCaseSensitiveChange,
    ]);

    return (
        <Tooltip
            title={(isDisabled) ?
                "" :
                "Match case"}
        >
            <Button
                color={"default"}
                disabled={isDisabled}
                size={"small"}
                icon={
                    <Typography.Text disabled={isDisabled}>
                        Aa
                    </Typography.Text>
                }
                variant={isCaseSensitive ?
                    "outlined" :
                    "text"}
                onClick={handleButtonClick}/>
        </Tooltip>
    );
};

export default CaseSensitiveToggle;
export type {CaseSensitiveToggleProps};
