# External database setup

This guide explains how to set up external databases for CLP instead of using the Docker Compose
managed databases. If the host(s) on which you're running CLP are ephemeral, you should use external
databases for metadata storage, and [object storage](guides-using-object-storage/index.md) for CLP's
archives and streams; this will ensure data is persisted even if a host is replaced.

:::{warning}
The [CLP Docker Compose project][docker-compose-orchestration] includes MariaDB/MongoDB databases by
default. This guide is only for users who want to customize their deployment by using their own
database servers or cloud-managed databases (e.g., [AWS RDS][aws-rds], [Azure
Database][azure-databases]).
:::

CLP requires two databases:

* **MariaDB/MySQL** - for storing metadata about archives, files, and jobs.
* **MongoDB** - for caching query results.

## MariaDB/MySQL setup

CLP is compatible with any MariaDB or MySQL database. The instructions below use Ubuntu as an
example, but you can use any compatible database installation or cloud-managed service.

### Installing MariaDB on Ubuntu

1. Install MariaDB server:

   ```bash
   sudo apt update
   sudo apt install mariadb-server
   ```

2. Connect to MariaDB as root:

   ```bash
   sudo mysql
   ```

3. Create the CLP database:

   ```sql
   CREATE DATABASE `clp-db`;
   ```

4. Create a user for CLP (replace `<password>` with a secure password):

   ```sql
   CREATE USER 'clp-user'@'%' IDENTIFIED BY '<password>';
   ```

   :::{note}
   The `'%'` allows connections from any host. For better security, replace `'%'` with the specific
   hostname or IP address from which CLP will connect (e.g., `'clp-user'@'192.168.1.10'`).
   :::

5. Grant privileges to the user:

   ```sql
   GRANT ALL PRIVILEGES ON `clp-db`.* TO 'clp-user'@'%';
   FLUSH PRIVILEGES;
   ```

6. Exit the MariaDB shell:

   ```sql
   EXIT;
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

### Verifying the MariaDB connection

You can verify the MariaDB connection by running:

```bash
mysql -h <mariadb-hostname-or-ip> -u clp-user -p clp-db
```

### Using AWS RDS for MariaDB/MySQL

When using AWS RDS:

1. Create a MariaDB or MySQL RDS instance in the AWS Console.
2. Note the endpoint hostname and port (the default is `3306`).
3. Create the database and user using a MySQL client:

   ```bash
   mysql -h <rds-endpoint> -u admin -p
   ```

   Then follow steps 2-5 from [Installing MariaDB on Ubuntu](#installing-mariadb-on-ubuntu).

4. Ensure the RDS security group allows inbound connections on port 3306 from your CLP hosts.

## MongoDB setup

CLP is compatible with any MongoDB database. For installation instructions, see the [MongoDB
installation documentation][mongodb-install].

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

## Configuring CLP to use external databases

After setting up your external databases, configure CLP to use them by editing `etc/clp-config.yaml`:

```yaml
database:
  host: "<mariadb-hostname-or-ip>"
  port: 3306
  name: "clp-db"
  # Credentials will be set in etc/credentials.yaml

results_cache:
  host: "<mongodb-hostname-or-ip>"
  port: 27017
  name: "clp-query-results"
```

Set the credentials in `etc/credentials.yaml`:

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

[aws-rds]: https://aws.amazon.com/rds/
[azure-databases]: https://azure.microsoft.com/en-us/products/category/databases
[docker-compose-orchestration]: ../dev-docs/design-deployment-orchestration.md#docker-compose-orchestration
[mongodb-install]: https://www.mongodb.com/docs/manual/installation/
[mongodb-security]: https://docs.mongodb.com/manual/security/
[multi-host-guide]: guides-docker-compose-deployment.md#starting-clp
