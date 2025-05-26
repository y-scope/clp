import {Type} from "@sinclair/typebox";


const ErrorSchema = Type.Object({
    message: Type.String(),
});

export {ErrorSchema};
