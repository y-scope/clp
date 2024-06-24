const Fastify = require("fastify");
const fastifyStatic = require("@fastify/static");
const path = require("path");


const authPlugin = require("./plugins/authPlugin");
const DbManager = require("./plugins/DbManager");

const LegacyRoutes = require("./routes/legacy");

// eslint-disable-next-line new-cap
const app = Fastify({logger: true});

app.register(authPlugin);

app.register(fastifyStatic, {
    root: path.join(__dirname, "client/dist"),
    prefix: "/log-viewer/",
});

app.register(LegacyRoutes);

/**
 * Parses environment variables into config values for the application.
 *
 * @return {object} containing config values including the SQL database credentials,
 * logs directory, and logging level.
 * @throws {Error} if the required environment variables are undefined, it exits the process with an
 * error.
 */
const parseEnvVars = () => {
    const {
        CLP_DB_USER,
        CLP_DB_PASS,
    } = process.env;
    const parsed = {
        CLP_DB_USER,
        CLP_DB_PASS,
    };

    for (const [varName, varValue] of Object.entries(parsed)) {
        if ("undefined" === typeof varValue) {
            console.error(`Environment variable ${varName} must be defined`);
            process.exit(1);
        }
    }

    return {
        ...parsed,
    };
};

/**
 * Starts the server and initializes database connection.
 */
const start = async () => {
    try {
        // TODO: parse command-line args for db ports
        const {CLP_DB_USER, CLP_DB_PASS} = parseEnvVars();
        app.register(DbManager, {
            mongo: {url: "mongodb://localhost:27017/clp-search"},
            mysql: {connectionString: `mysql://${CLP_DB_USER}:${CLP_DB_PASS}@localhost/clp-db`},
        });

        await app.listen({port: 3000});
        app.log.info(`Server listening on ${app.server.address().port}`);
    } catch (err) {
        app.log.error(err);
        process.exit(1);
    }
};

start();
