import {useCallback} from "react";

import {
    Button,
    Tooltip,
    Typography,
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
                color={"default"}
                disabled={disabled}
                size={"small"}
                icon={
                    <Typography.Text disabled={disabled}>
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


export type {CaseSensitiveToggleProps};
export default CaseSensitiveToggle;
