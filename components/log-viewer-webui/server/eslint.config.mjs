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
            "@typescript-eslint/require-await": [
                // Fastify recommends async syntax, but not all plugins require Promise resolution
                // in their function bodies.
                "off",
            ],
            "@typescript-eslint/no-floating-promises": [
                "error",
                {
                    allowForKnownSafeCalls: [
                        {
                            from: "package",
                            name: "test",
                            package: "tap"
                        },
                    ],
                },
            ],
            "new-cap": [
                "error",
                {
                    // TypeBox imports
                    capIsNewExceptions: [
                        "Type.Enum",
                        "Type.Integer",
                    ],
                },
            ],
        },
    },
];


export default EslintConfig;
