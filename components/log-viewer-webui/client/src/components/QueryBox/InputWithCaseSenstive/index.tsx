import {
    Input,
    InputProps,
} from "antd";

import CaseSensitiveToggle, {CaseSensitiveToggleProps} from "./CaseSenstiveToggle";


/**
 * Antd Input props and case sensitive toggle props with suffix omitted since set by component.
 */
type InputWithCaseSensitiveProps = Omit<InputProps, "suffix"> & CaseSensitiveToggleProps;

/**
 * An input component with a built-in case sensitivity toggle.
 *
 * @param props
 * @param props.inputProps
 * @param props.isCaseSensitive
 * @param props.onCaseSensitiveChange
 * @return
 */
const InputWithCaseSensitive = ({
    isCaseSensitive,
    onCaseSensitiveChange,
    ...inputProps
}: InputWithCaseSensitiveProps) => {
    return (
        <Input
            {...inputProps}
            suffix={
                <CaseSensitiveToggle
                    disabled={inputProps.disabled}
                    isCaseSensitive={isCaseSensitive}
                    onCaseSensitiveChange={onCaseSensitiveChange}/>
            }/>
    );
};

export default InputWithCaseSensitive;
export type {InputWithCaseSensitiveProps};
