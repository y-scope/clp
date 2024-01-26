# Setup

## Requirements

* [NodeJS 14](https://nodejs.org/download/release/v14.21.3/)
* [Meteor](https://docs.meteor.com/install.html#installation)

## Install the dependencies

```shell
meteor npm install
```

If you ever add a package manually to `package.json` or `package.json` changes
for some other reason, you should rerun this command.

# Running in development

The full functionality of the webui depends on other components in the CLP
package:

* Build the CLP package
* Start the package: `<clp-package>/sbin/start-clp`
* Then run `<clp-package>/sbin/stop-clp webui` to stop the webui instance started
  in the package.
* Start meteor:

  ```shell
  MONGO_URL="mongodb://localhost:27017/clp-search" \
  ROOT_URL="http://<webui_host>"                   \
  CLP_DB_HOST="localhost"                          \
  CLP_DB_PORT=3306                                 \
  CLP_DB_NAME="clp-db"                             \
  CLP_DB_USER="clp-user"                           \
  CLP_DB_PASS="<clp-password>"                     \
    meteor --port <webui_port> --settings settings.json
  ```
  
  * Change `<host>` to the IP of the machine you're running the webui on.

* The webui should now be available at `http://<host>:3000`
