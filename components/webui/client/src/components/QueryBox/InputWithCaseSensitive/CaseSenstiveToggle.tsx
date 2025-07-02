import {useCallback} from "react";

import {
    Button,
    Tooltip,
} from "antd";


interface CaseSensitiveToggleProps {
    disabled: boolean;
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
    const handleButtonClick = useCallback(() => {
        onCaseSensitiveChange(!isCaseSensitive);
    }, [
        isCaseSensitive,
        onCaseSensitiveChange,
    ]);

    return (
        <Tooltip
            title={"Match case"}
        >
            <Button
                disabled={disabled}
                size={"small"}
                color={isCaseSensitive ?
                    "primary" :
                    "default"}
                variant={isCaseSensitive ?
                    "solid" :
                    "outlined"}
                onClick={handleButtonClick}
            >
                Aa
            </Button>
        </Tooltip>
    );
};


export type {CaseSensitiveToggleProps};
export default CaseSensitiveToggle;
