import {Table} from "antd";

import {
    SearchResult,
    searchResultsTableColumns,
} from "./typings";

import {useSearchResults} from "../../reactive-mongo-queries/useSearchResults";

/**
 * Renders search results in a table.
 *
 * @return
 */
const SearchResultsTable = () => {
    let searchResults = useSearchResults();

    return (
        <Table<SearchResult>
            scroll={{ y: 400 }}
            columns={searchResultsTableColumns}
            // Render empty result array while subscription is pending or if there is no active
            // query.
            dataSource={searchResults ? searchResults : []}
            pagination={false}
            rowKey={(record) => record._id.toString()}
            virtual={true}/>
    );
};

export default SearchResultsTable;
