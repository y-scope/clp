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
            columns={searchResultsTableColumns}
            pagination={false}
            rowKey={(record) => record._id.toString()}
            scroll={{y: 400}}

            virtual={true}
            dataSource={
                searchResults ?
                    searchResults :
                    []
            }/>
    );
};

export default SearchResultsTable;
