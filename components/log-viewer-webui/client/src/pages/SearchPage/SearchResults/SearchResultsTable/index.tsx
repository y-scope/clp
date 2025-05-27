import {Table} from "antd";

import {useSearchResults} from "../../reactive-mongo-queries/useSearchResults";
import {
    SearchResult,
    searchResultsTableColumns,
} from "./typings";


/**
 * Renders search results in a table.
 *
 * @return
 */
const SearchResultsTable = () => {
    const searchResults = useSearchResults();

    return (
        <Table<SearchResult>
            dataSource={searchResults ? searchResults : []}
            columns={searchResultsTableColumns}
            // Render empty result array while subscription is pending or if there is no active
            // query.
            pagination={false}
            rowKey={(record) => record._id.toString()}
            scroll={{ y: 400 }}
            virtual={true}/>
    );
};

export default SearchResultsTable;
