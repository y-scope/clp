import HtmlWebpackPlugin from "html-webpack-plugin";
import MiniCssExtractPlugin from "mini-css-extract-plugin";
import * as path from "node:path";
import {fileURLToPath} from "node:url";

import ReactRefreshPlugin from "@pmmmwh/react-refresh-webpack-plugin";


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
    devServer: {
        proxy: [
            {
                context: ["/"],
                target: "http://localhost:3000",
            },
        ],
    },
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
                        plugins: isProduction ?
                            [] :
                            ["react-refresh/babel"],
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
        filename: isProduction ?
            "[name].[contenthash].bundle.js" :
            "[name].bundle.js",
        clean: true,
        publicPath: "auto",
    },
    plugins: plugins.concat(isProduction ?
        [new MiniCssExtractPlugin()] :
        [new ReactRefreshPlugin()]),
    resolve: {
        extensions: [
            ".js",
            ".jsx",
            ".json",
        ],
    },
};

export default () => {
    config.mode = isProduction ?
        "production" :
        "development";

    return config;
};
