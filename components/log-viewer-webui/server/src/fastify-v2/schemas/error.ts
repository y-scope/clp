import { Type } from '@sinclair/typebox'




// Base error schema — minimal
 const BaseErrorSchema = Type.Object({
    message: Type.String(),
  });

  // Extended error — adds code and statusCode
   const FastifyErrorSchema = Type.Intersect([
    BaseErrorSchema,
    Type.Object({
        code: Type.String(),
        name: Type.String(),
        statusCode: Type.Optional(Type.Integer({ minimum: 100, maximum: 599 })),
    }),
  ]);


  export {BaseErrorSchema, FastifyErrorSchema};