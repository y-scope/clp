import { useCallback } from "react";
import { Button, Tooltip, Typography } from "antd";

export interface CaseSensitiveToggleProps {
    isCaseSensitive: boolean;
    onCaseSensitiveChange: (newValue: boolean) => void;
    disabled?: boolean | undefined;
}

/**
 * A toggle button component that switches between case-sensitive and case-insensitive modes.
 *
 * This component displays a button labeled "Aa". When clicked, it toggles the `isCaseSensitive`
 * state and invokes the `onCaseSensitiveChange` callback with the new value.
 *
 * @param props
 * @param props.isCaseSensitive
 * @param props.onCaseSensitiveChange
 * @param props.disabled
 * @return
 */
const CaseSensitiveToggle = ({
    isCaseSensitive,
    onCaseSensitiveChange,
    disabled,
}: CaseSensitiveToggleProps) => {
    const isDisabled = disabled ?? false;

    const handleButtonClick = useCallback(() => {
        if (!isDisabled) {
            onCaseSensitiveChange(!isCaseSensitive);
        }
    }, [
        isCaseSensitive,
        onCaseSensitiveChange,
    ]);

    return (
        <Tooltip title={(isDisabled)? "": "Match case"}>
            <Button
                color={"default"}
                icon={
                    <Typography.Text disabled={isDisabled}>
                        Aa
                    </Typography.Text>
                }
                size={"small"}
                variant={isCaseSensitive ?
                    "outlined" :
                    "text"}
                disabled={isDisabled}
                onClick={handleButtonClick}/>
        </Tooltip>
    );
};

export default CaseSensitiveToggle;
