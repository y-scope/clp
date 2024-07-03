import HtmlWebpackPlugin from "html-webpack-plugin";
import MiniCssExtractPlugin from "mini-css-extract-plugin";
import * as path from "node:path";
import {fileURLToPath} from "node:url";


const filename = fileURLToPath(import.meta.url);
const dirname = path.dirname(filename);

const isProduction = "production" === process.env.NODE_ENV;

const stylesHandler = isProduction ?
    MiniCssExtractPlugin.loader :
    "style-loader";

const plugins = [
    new HtmlWebpackPlugin({
        template: path.resolve(dirname, "public", "index.html"),
    }),
];

const config = {
    devtool: isProduction ?
        "source-map" :
        "eval-source-map",
    entry: path.resolve(dirname, "src", "index.jsx"),
    module: {
        rules: [
            {
                test: /\.(js|jsx)$/i,
                exclude: /node_modules/,
                use: {
                    loader: "babel-loader",
                    options: {
                        presets: [
                            "@babel/preset-env",
                            [
                                "@babel/preset-react",
                                {
                                    runtime: "automatic",
                                },
                            ],
                        ],
                    },
                },
            },
            {
                test: /\.css$/i,
                use: [
                    stylesHandler,
                    "css-loader",
                ],
            },
        ],
    },
    output: {
        path: path.resolve(dirname, "dist"),
        filename: "[name].[contenthash].bundle.js",
        clean: true,
        publicPath: "auto",
    },
    plugins: plugins.concat(isProduction ?
        [new MiniCssExtractPlugin()] :
        []),
    resolve: {
        extensions: [
            ".js",
            ".jsx",
            ".json",
        ],
    },
};

export default () => {
    if (isProduction) {
        config.mode = "production";
    } else {
        config.mode = "development";
    }

    return config;
};
