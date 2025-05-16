import {Type} from "@sinclair/typebox";


export const StringSchema = Type.String({
    minLength: 1,
});

export const IdSchema = Type.Integer({minimum: 1});
