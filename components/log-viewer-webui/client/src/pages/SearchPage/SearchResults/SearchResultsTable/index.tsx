import {Table} from "antd";

import {
    SearchResult,
    searchResultsTableColumns,
} from "./typings";

import {useSearchResults} from "../../../SearchPage/searchResults";

/**
 * Renders search results in a table.
 *
 * @return
 */
const SearchResultsTable = () => {

    let searchResults = useSearchResults();
    console.log("searchResults", searchResults);

    return (
        <Table<SearchResult>
            scroll={{ y: 400 }}
            columns={searchResultsTableColumns}
            dataSource={searchResults as SearchResult[]}
            pagination={false}
            rowKey={(record) => record._id.toString()}
            virtual={true}/>
    );
};

export default SearchResultsTable;
