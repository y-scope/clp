import {useRef} from "react";

let currentDataset = "default";

export const useDatasetValue = () => {
    const setDataset = (value: string) => {
        currentDataset = value;
    };

    const getDataset = () => currentDataset;

    return { setDataset, getDataset };
};
