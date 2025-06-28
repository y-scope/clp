import CommonConfig from "eslint-config-yscope/CommonConfig.mjs";
import ReactConfigArray from "eslint-config-yscope/ReactConfigArray.mjs";
import StylisticConfigArray from "eslint-config-yscope/StylisticConfigArray.mjs";
import TsConfigArray, {createTsConfigOverride} from "eslint-config-yscope/TsConfigArray.mjs";


const EslintConfig = [
    {
        ignores: [
            "dist/",
            "node_modules/",
        ],
    },
    CommonConfig,
    ...TsConfigArray,
    createTsConfigOverride(
        [
            "src/**/*.ts",
            "src/**/*.tsx",
        ],
        "./tsconfig/tsconfig.app.json"
    ),
    createTsConfigOverride(
        ["vite.config.ts"],
        "./tsconfig/tsconfig.node.json"
    ),
    ...StylisticConfigArray,
    ...ReactConfigArray,
    {
        rules: {
            "new-cap": [
                "error",
                {
                    capIsNewExceptions: [
                        // TypeBox imports
                        "Decode",
                        "Encode",
                        "Type.Transform",
                        "Type.Union",
                        "Type.Literal",
                        "Value.Parse",
                    ],
                },
            ],
        },
    },
    {
        // eslint-disable-next-line no-warning-comments
        // TODO: Remove dot notation rule once part of eslint-config-yscope
        files: [
            "**/*.ts",
            "**/*.tsx",
        ],
        rules: {
            "dot-notation": "off",
            "@typescript-eslint/dot-notation": "error",
        },
    },
];


export default EslintConfig;
