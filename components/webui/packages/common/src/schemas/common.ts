import {Type} from "@sinclair/typebox";


const StringSchema = Type.String({
    minLength: 1,
});

const IdSchema = Type.Integer({minimum: 1});

export {
    IdSchema,
    StringSchema,
};
