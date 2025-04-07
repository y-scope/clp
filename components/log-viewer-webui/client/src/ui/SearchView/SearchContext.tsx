import React, {
    createContext,
    useState,
} from "react";

import dayjs from "dayjs";


interface SearchContextType {
    queryString: string;
    timeRange: [dayjs.Dayjs, dayjs.Dayjs];

    setQueryString: (query: string) => void;
    setTimeRange: (range: [dayjs.Dayjs, dayjs.Dayjs]) => void;
}

/**
 * Default values of the search context.
 */
const SEARCH_CONTEXT_DEFAULT: Readonly<SearchContextType> = Object.freeze({
    queryString: "",

    // Type casting since typescript cannot infer the correct type for array literals
    timeRange: [dayjs().startOf("day"),
        dayjs().endOf("day")] as [dayjs.Dayjs, dayjs.Dayjs],
    setQueryString: () => null,
    setTimeRange: () => null,
});

const SearchContext = createContext<SearchContextType>(SEARCH_CONTEXT_DEFAULT);

interface SearchContextProviderProps {
    children: React.ReactNode;
}

/**
 * Provider for the search context.
 *
 * @param props
 * @param props.children
 * @return
 */
const SearchContextProvider = ({children}: SearchContextProviderProps) => {
    const [queryString, setQueryString] = useState<string>(SEARCH_CONTEXT_DEFAULT.queryString);
    const [timeRange, setTimeRange] =
        useState<[dayjs.Dayjs, dayjs.Dayjs]>(SEARCH_CONTEXT_DEFAULT.timeRange);

    return (
        <SearchContext.Provider
            value={{
                queryString,
                timeRange,
                setQueryString,
                setTimeRange,
            }}
        >
            {children}
        </SearchContext.Provider>
    );
};

export default SearchContextProvider;
export {
    SEARCH_CONTEXT_DEFAULT,
    SearchContext,
};
