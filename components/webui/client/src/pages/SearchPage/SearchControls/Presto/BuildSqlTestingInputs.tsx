import {FormEvent} from "react";

import {
    buildSearchQuery,
    BuildSearchQueryProps,
} from "../../../../sql-parser";
import {handlePrestoQuerySubmit} from "./presto-search-requests";


// eslint-disable-next-line no-warning-comments
// TODO: Replace this with actual SQL inputs

/**
 * Returns a temporary testing component.
 *
 * @return
 */
const BuildSqlTestingInputs = () => {
    return (
        <form
            onSubmit={(ev: FormEvent<HTMLFormElement>) => {
                ev.preventDefault();
                const formData = new FormData(ev.target as HTMLFormElement);
                const props = Object.fromEntries(formData) as unknown as BuildSearchQueryProps;
                const sqlString = buildSearchQuery(props);
                console.log(`SQL: ${sqlString}`);
                handlePrestoQuerySubmit({queryString: sqlString});
            }}
        >
            <label>select:</label>
            <input name={"selectItemList"}/>
            <label>from:</label>
            <input name={"relationList"}/>
            <label>where:</label>
            <input name={"booleanExpression"}/>
            <label>order:</label>
            <input name={"sortItemList"}/>
            <label>limit:</label>
            <input name={"limitValue"}/>
            <button type={"submit"}>
                Run
            </button>
        </form>
    );
};

export {BuildSqlTestingInputs};
