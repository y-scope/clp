import dotenv from "dotenv";


/**
 * Parses environment variables into config values for the application.
 *
 * @return {{HOST: string, PORT: string, CLP_DB_USER: string, CLP_DB_PASS: string}}
 * @throws {Error} if any required environment variable is undefined.
 */
const parseEnvVars = () => {
    dotenv.config({
        path: ".env",
    });

    const {
        HOST, PORT, CLP_DB_USER, CLP_DB_PASS
    } = process.env;
    const envVars = {
        HOST, PORT, CLP_DB_USER, CLP_DB_PASS
    };

    // Check for mandatory environment variables
    for (const [key, value] of Object.entries(envVars)) {
        if ("undefined" === typeof value) {
            throw new Error(`Environment variable ${key} must be defined.`);
        }
    }

    return envVars;
};

export {parseEnvVars};