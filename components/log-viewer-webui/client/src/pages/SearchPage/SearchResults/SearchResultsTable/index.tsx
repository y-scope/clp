import {Table} from "antd";

import {
    SearchResult,
    searchResultsTableColumns,
} from "./typings";

import {useSearchResults} from "../../SearchState/mongo-queries/useSearchResults";

/**
 * Renders search results in a table.
 *
 * @return
 */
const SearchResultsTable = () => {

    let searchResults = useSearchResults();
    console.log("searchResults", searchResults);

// If the state is not done. return empty array.
// when it is done. 
    return (
        <Table<SearchResult>
            scroll={{ y: 400 }}
            columns={searchResultsTableColumns}
            dataSource={searchResults ? searchResults : []}
            pagination={false}
            rowKey={(record) => record._id.toString()}
            virtual={true}/>
    );
};

export default SearchResultsTable;
