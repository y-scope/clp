import { FC } from "react";

export interface QueryBoxProps {
    placeholder?: string;
    progress?: number;
    size?: "large" | "small" | "default";
    value: string;
    onChange: (e: React.ChangeEvent<HTMLInputElement>) => void;
}

declare const QueryBox: FC<QueryBoxProps>;
export default QueryBox;
