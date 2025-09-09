import CommonConfig from "eslint-config-yscope/CommonConfig.mjs";
import StylisticConfigArray from "eslint-config-yscope/StylisticConfigArray.mjs";
import TsConfigArray from "eslint-config-yscope/TsConfigArray.mjs";


const EslintConfig = [
    {
        ignores: [
            "dist/",
            "node_modules/",
        ],
    },
    CommonConfig,
    ...TsConfigArray,
    ...StylisticConfigArray,
    {
        rules: {
            "new-cap": [
                "error",
                {
                    // TypeBox imports
                    capIsNewExceptions: [
                        "Type.Any",
                        "Type.Enum",
                        "Type.Integer",
                        "Type.Literal",
                        "Type.Null",
                        "Type.Number",
                        "Type.Object",
                        "Type.Optional",
                        "Type.Required",
                        "Type.String",
                        "Type.Union",
                        "Value.Errors",
                        "Value.Parse",
                    ],
                },
            ],
        },
    },
    {
        files: [
            "**/*.ts",
        ],
        rules: {
            "@typescript-eslint/no-floating-promises": [
                "error",
                {
                    allowForKnownSafeCalls: [
                        {
                            from: "package",
                            name: "test",
                            package: "tap",
                        },
                    ],
                },
            ],
            "@typescript-eslint/require-await": [
                // Fastify recommends async syntax, but not all plugins require Promise resolution
                // in their function bodies.
                "off",
            ],
        },
    },
];


export default EslintConfig;
