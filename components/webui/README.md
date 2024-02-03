# Setup

## Requirements

* [Node.js 14](https://nodejs.org/download/release/v14.21.3/): Meteor.js only runs on that specific version. 
* [Meteor.js](https://docs.meteor.com/install.html#installation)

## Install the dependencies

```shell
meteor npm install
```

If you ever add a package manually to `package.json` or `package.json` changes
for some other reason, you should rerun this command.

# Running in development

The full functionality of the webui depends on other components in the CLP
package:

1. Build the CLP package
2. Start the package: `<clp-package>/sbin/start-clp.sh`
3. Then run `<clp-package>/sbin/stop-clp.sh webui` to stop the webui instance started
  in the package.
4. Start meteor:
   ```shell
   MONGO_URL="mongodb://localhost:<results_cache.port>/<results_cache.db_name>" \
   ROOT_URL="http://<webui.host>"                                               \
   CLP_DB_HOST="<database.host>"                                                \
   CLP_DB_PORT=<database.port>                                                  \
   CLP_DB_NAME="<database.name>"                                                \
   CLP_DB_USER="<database.user>"                                                \
   CLP_DB_PASS="<database.password>"                                            \
     meteor --port <webui.port> --settings settings.json
   ```
   
   Here is an example according to `package-template`:
   ```shell
   # Please update `<database.password>` accordingly
   
   MONGO_URL="mongodb://localhost:27017/clp-search" \
   ROOT_URL="http://localhost"                      \
   CLP_DB_HOST="localhost"                          \
   CLP_DB_PORT=3306                                 \
   CLP_DB_NAME="clp-db"                             \
   CLP_DB_USER="clp-user"                           \
   CLP_DB_PASS="<database.password>"                \
     meteor --port 4000 --settings settings.json
   ```
5. The Web UI should now be available at `http://<webui.host>:<webui.port>` (e.g., http://localhost:3000).
