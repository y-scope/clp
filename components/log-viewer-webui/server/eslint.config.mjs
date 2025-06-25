import CommonConfig from "eslint-config-yscope/CommonConfig.mjs";
import StylisticConfigArray from "eslint-config-yscope/StylisticConfigArray.mjs";
import TsConfigArray from "eslint-config-yscope/TsConfigArray.mjs";


const EslintConfig = [
    {
        ignores: [
            "dist/",
            "yscope-log-viewer/dist/",
            "client/dist/",
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
                        "Type.Enum",
                        "Type.Integer",
                        "Type.Literal",
                        "Type.Null",
                        "Type.Required",
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
