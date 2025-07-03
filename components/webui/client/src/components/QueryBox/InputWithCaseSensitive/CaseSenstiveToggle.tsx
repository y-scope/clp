import {useCallback} from "react";

import {
    Button,
    ButtonProps,
    Tooltip,
} from "antd";


interface CaseSensitiveToggleProps {
    disabled: boolean;
    isCaseSensitive: boolean;
    onCaseSensitiveChange: (newValue: boolean) => void;
}

type ButtonVariantType = ButtonProps["variant"];

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
    const handleButtonClick = useCallback(() => {
        onCaseSensitiveChange(!isCaseSensitive);
    }, [
        isCaseSensitive,
        onCaseSensitiveChange,
    ]);

    let buttonVariant: ButtonVariantType = "outlined";
    if (isCaseSensitive) {
        buttonVariant = "solid";
    } else if (disabled) {
        // The "solid" and "outlined" variants look the same when disabled,
        // so we use "text" variant for a different appearance.
        buttonVariant = "text";
    }

    return (
        <Tooltip
            title={"Match case"}
        >
            <Button
                disabled={disabled}
                size={"small"}
                variant={buttonVariant}
                color={isCaseSensitive ?
                    "primary" :
                    "default"}
                onClick={handleButtonClick}
            >
                Aa
            </Button>
        </Tooltip>
    );
};


export type {CaseSensitiveToggleProps};
export default CaseSensitiveToggle;
