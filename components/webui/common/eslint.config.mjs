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
                        "Type.Required",
                        "Type.Union",
                        "Value.Errors",
                        "Value.Parse",
                    ],
                },
            ],
        },
    },
];


export default EslintConfig;
