# External database setup

This guide explains how to set up external databases for CLP instead of using the bundled
databases. If the host(s) on which you're running CLP are ephemeral, you should use external
databases for metadata storage, and [object storage](guides-using-object-storage/index.md) for CLP's
archives and streams; this will ensure data is persisted even if a host is replaced.

:::{warning}
Both the [CLP Docker Compose project][docker-compose-orchestration] and the
[CLP Helm chart][k8s-guide] include MariaDB/MongoDB databases by default. This guide is only for
users who want to customize their deployment by using their own database servers or cloud-managed
databases (e.g., [AWS RDS][aws-rds], [Azure Database][azure-databases]).
:::

CLP requires two types of databases:

* **MariaDB/MySQL** - for storing:
  * metadata about CLP's archives, files, compression jobs, and query jobs.
  * metadata about Spider's jobs (only when Spider is used for scheduling).
* **MongoDB** - for caching query results.

## MariaDB/MySQL setup

CLP is compatible with any MariaDB or MySQL database. The instructions below use Ubuntu as an
example, but you can use any compatible database installation or cloud-managed service.

### Installing MariaDB on Ubuntu

Install MariaDB server:

```bash
sudo apt update
sudo apt install mariadb-server
```

### Configuring MariaDB for remote connections

If CLP components will connect from a different host, you need to configure MariaDB to accept remote
connections:

1. Edit the MariaDB configuration file:

   ```bash
   sudo nano /etc/mysql/mariadb.conf.d/50-server.cnf
   ```

2. Find the `bind-address` line and change it to allow connections from all interfaces:

   ```ini
   bind-address = 0.0.0.0
   ```

3. Restart MariaDB:

   ```bash
   sudo systemctl restart mariadb
   ```

### Using AWS RDS for MariaDB/MySQL

When using AWS RDS:

1. Create a MariaDB or MySQL RDS instance in the AWS Console.
2. Note the endpoint hostname and port (the default is `3306`).
3. Ensure the RDS security group allows inbound connections on port 3306 from your CLP hosts.

You can then connect to the instance with `mysql -h <rds-endpoint> -u admin -p` and follow the
database and user creation steps in
[Using an external database with CLP](#using-an-external-database-with-clp).

## MongoDB setup

CLP is compatible with any MongoDB database. For installation instructions, see the [MongoDB
installation documentation][mongodb-install].

:::{warning}
Running an external MongoDB on the **same host** as CLP (i.e., using `localhost` or `127.0.0.1` as
the `results_cache` host) is not supported. CLP's `results-cache-indices-creator` initializes a
MongoDB replica set using the configured hostname, which MongoDB must be able to resolve to itself;
`localhost` from inside a Docker container does not resolve to the host machine.

Instead, either:

* Keep `results_cache` in the `bundled` list (recommended for single-host deployments).
* Use a truly remote MongoDB instance and specify its hostname or IP.
* If you must use a same-host MongoDB, configure `results_cache.host` in `clp-config.yaml` to the
  host's non-loopback IP address (e.g., `192.168.1.10`) and ensure MongoDB is bound to that
  address.
:::

### Creating the CLP database in MongoDB

MongoDB automatically creates databases and collections when first accessed, so no manual database
creation is needed. CLP will create the necessary database and collections (`clp-query-results` by
default) when it first connects.

### Configuring MongoDB for remote connections

If CLP components will connect from a different host:

1. Edit the MongoDB configuration file:

   ```bash
   sudo nano /etc/mongod.conf
   ```

2. Find the `net.bindIp` setting and change it to allow connections from all interfaces:

   ```yaml
   net:
     port: 27017
     bindIp: 0.0.0.0
   ```

3. Restart MongoDB:

   ```bash
   sudo systemctl restart mongod
   ```

:::{warning}
For production deployments, it's highly recommended to enable authentication and SSL/TLS for
MongoDB. See the [MongoDB security documentation][mongodb-security] for details.
:::

### Verifying the MongoDB connection

You can verify the MongoDB connection by running:

```bash
mongosh "mongodb://<mongodb-hostname-or-ip>:27017/clp-query-results"
```

### Using AWS DocumentDB or MongoDB Atlas

When using AWS DocumentDB or MongoDB Atlas:

1. Create a cluster in the AWS Console or MongoDB Atlas.
2. Note the connection string/endpoint provided.
3. Ensure the security group or IP access list allows connections from your CLP hosts.
4. Use the provided connection string when configuring CLP (see below).

## Using an external database with CLP

First, create a database and user for CLP on your MariaDB/MySQL server:

1. Connect to MariaDB as root:

   ```bash
   sudo mysql
   ```

2. Create the CLP database:

   ```sql
   CREATE DATABASE `clp-db`;
   ```

3. Create a user for CLP (replace `<password>` with a secure password):

   ```sql
   CREATE USER 'clp-user'@'%' IDENTIFIED BY '<password>';
   ```

   :::{note}
   The `'%'` allows connections from any host. For better security, replace `'%'` with the specific
   hostname or IP address from which CLP will connect (e.g., `'clp-user'@'192.168.1.10'`).
   :::

4. Grant privileges to the user:

   ```sql
   GRANT ALL PRIVILEGES ON `clp-db`.* TO 'clp-user'@'%';
   FLUSH PRIVILEGES;
   ```

5. Exit the MariaDB shell:

   ```sql
   EXIT;
   ```

You can verify the connection by running:

```bash
mysql -h <mariadb-hostname-or-ip> -u clp-user -p clp-db
```

Then configure CLP to use the external databases, depending on how you deploy it:

### Configuring with Docker Compose

1. Edit `etc/clp-config.yaml` to specify which services are bundled (managed by the
   [CLP Docker Compose project][docker-compose-orchestration]):

   ```yaml
   # Remove "database" and "results_cache" from this list to use external instances
   bundled:
     # - "database"
     - "queue"
     - "redis"
     # - "results_cache"
     - "otel_collector"
   ```

2. Configure the connection details for your external databases in `etc/clp-config.yaml`:

   ```yaml
   database:
     host: "<mariadb-hostname-or-ip>"
     port: <mariadb-port>

   results_cache:
     host: "<mongodb-hostname-or-ip>"
     port: <mongodb-port>
   ```

3. Set the credentials in `etc/credentials.yaml`:

   ```yaml
   database:
     username: "clp-user"
     password: "<your-mariadb-password>"
   ```

:::{note}
When using external databases in a multi-host deployment, you do **not** need to start the
`database` and `results-cache` Docker Compose services. Skip those services when following the
[multi-host deployment guide][multi-host-guide]. However, you still need to run the database
initialization jobs (`db-table-creator` and `results-cache-indices-creator`).
:::

### Configuring with the Helm chart

Configure the external databases in your Helm values file:

```{code-block} yaml
:caption: external-database-values.yaml

clpConfig:
  # Remove "database" and "results_cache" from this list to use external instances.
  bundled:
    # - "database"
    - "queue"
    - "redis"
    # - "results_cache"
    - "otel_collector"
    - "presto"

  database:
    type: "mariadb"  # "mariadb" or "mysql"
    host: "<mariadb-hostname-or-ip>"
    port: <mariadb-port>

  results_cache:
    host: "<mongodb-hostname-or-ip>"
    port: <mongodb-port>

credentials:
  database:
    username: "clp-user"
    password: "<your-mariadb-password>"
```

## Using an external database with Spider

1. Create the Spider database on your MariaDB/MySQL server:

   ```sql
   CREATE DATABASE `spider-db`;
   ```

2. Grant privileges on the database. If Spider shares the same database server as CLP, you don't
   need a separate user - grant the CLP user privileges on the Spider database:

   ```sql
   GRANT ALL PRIVILEGES ON `spider-db`.* TO 'clp-user'@'%';
   FLUSH PRIVILEGES;
   ```

   Otherwise, create a dedicated user first, following the same steps as in
   [Using an external database with CLP](#using-an-external-database-with-clp).

3. Configure the Spider subchart to use the external database:

   ```{code-block} yaml
   :caption: external-database-values.yaml (continued)

   spider:
     enabled: true
     spiderConfig:
       # Remove "database" from this list to use external instances.
       bundled: [
         # "database"
       ]
       database:
         host: "<mariadb-hostname-or-ip>"
         port: <mariadb-port>
         name: "spider-db"
         username: "clp-user"
         password: "<your-mariadb-password>"
   ```

[aws-rds]: https://aws.amazon.com/rds/
[azure-databases]: https://azure.microsoft.com/en-us/products/category/databases
[docker-compose-orchestration]: guides-docker-compose-deployment.md
[k8s-guide]: guides-k8s-deployment.md
[mongodb-install]: https://www.mongodb.com/docs/manual/installation/
[mongodb-security]: https://docs.mongodb.com/manual/security/
[multi-host-guide]: guides-docker-compose-deployment.md#starting-clp
