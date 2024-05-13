import assert from "assert";

import {unquoteString} from "/imports/utils/misc";


describe("misc", () => {
    it("unquoteString", () => {
        // Empty string
        assert.strictEqual(unquoteString("", '"', "\\"), "");

        // Unquoted string
        assert.strictEqual(unquoteString("abc", '"', "\\"), "abc");

        // Double-quoted string
        assert.strictEqual(unquoteString("\"abc\"", '"', "\\"), "abc");

        // Single-quoted string
        assert.strictEqual(unquoteString("'abc'", "'", "\\"), "abc");

        // With escaped characters
        assert.strictEqual(unquoteString("a\\\\b\\\"c\\*\\?", '"', "\\"), "a\\\\b\"c\\*\\?");

        // Double-quoted with escaped characters
        assert.strictEqual(unquoteString("\"a\\\\b\\\"c\\*\\?\"", '"', "\\"), "a\\\\b\"c\\*\\?");

        // With one of the quotes missing
        assert.throws(() => {
            unquoteString("\"abc", '"', "\\");
        }, Error);
        assert.throws(() => {
            unquoteString("abc\"", '"', "\\");
        }, Error);

        // With an unescaped quote in the middle
        assert.throws(() => {
            unquoteString("ab\"c", '"', "\\");
        }, Error);
        assert.throws(() => {
            unquoteString("\"ab\"c\"", '"', "\\");
        }, Error);
    });
});
