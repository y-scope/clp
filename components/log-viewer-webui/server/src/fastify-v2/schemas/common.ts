import {Type, TSchema} from "@sinclair/typebox";


const StringSchema = Type.String({
    minLength: 1,
});

const IdSchema = Type.Integer({minimum: 1});

const Nullable = <T extends TSchema>(T: T) => {
  return Type.Union([T, Type.Null()])
}

export {
    Nullable,
    IdSchema,
    StringSchema,
};
