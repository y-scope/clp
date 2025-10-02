import {
    useCallback,
    useEffect,
    useRef,
    useState,
} from "react";

import {Nullable} from "@webui/common/utility-types";


/**
 * Pseudo progress bar increment on each interval tick.
 */
const PROGRESS_INCREMENT = 5;

/**
 * The interval for updating the pseudo progress bar.
 */
const PROGRESS_INTERVAL_MILLIS = 100;

/**
 * A custom hook to manage a pseudo progress value.
 * The progress increases gradually and can be reset.
 *
 * @return
 */
const usePseudoProgress = (): {
    progress: Nullable<number>;
    start: () => void;
    stop: () => void;
} => {
    const [progress, setProgress] = useState<Nullable<number>>(null);
    const intervalIdRef = useRef<number>(0);

    const start = useCallback(() => {
        if (0 !== intervalIdRef.current) {
            console.warn("Interval already set for submitted query");

            return;
        }
        intervalIdRef.current = window.setInterval(() => {
            setProgress((v) => {
                if (100 <= (v ?? 0) + PROGRESS_INCREMENT) {
                    return 100;
                }

                return (v ?? 0) + PROGRESS_INCREMENT;
            });
        }, PROGRESS_INTERVAL_MILLIS);
    }, []);

    const stop = useCallback(() => {
        clearInterval(intervalIdRef.current);
        intervalIdRef.current = 0;
        setProgress(null);
    }, []);

    useEffect(() => {
        return () => {
            if (0 !== intervalIdRef.current) {
                clearInterval(intervalIdRef.current);
                intervalIdRef.current = 0;
            }
        };
    }, []);

    return {
        progress,
        start,
        stop,
    };
};

export {usePseudoProgress};
