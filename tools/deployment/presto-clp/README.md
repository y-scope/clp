# Setup local docker stack for presto + clp

## Install docker

Follow the guide here: [docker]

# Launch clp-package

1. Find the clp-package for test on our official website [clp-json-v0.4.0]. Here is a sample dataset for demo testing: [postgresql dataset].

2. Untar the clp-package and the postgresql dataset.

3. Replace the content of `/path/to/clp-json-package/etc/clp-config.yml` with the output of `demo-assets/init.sh generate_sample_clp_config`.

4. Launch:

```bash
# You probably want to run a python 3.9 or newer virtual environment
sbin/start-clp.sh
```

5. Compress:

```bash
# You can also use your own dataset
sbin/compress.sh --timestamp-key 'timestamp' /path/to/postgresql.log
```

6. Use the following command to update `.env`:

```bash
demo-assets/init.sh update_metadata_config /path/to/clp-json-package
```

# Create Docker Cluster

Create a local docker stack:

```bash
docker compose up
```

To create a docker stack with more than 1 worker (e.g., 3 workers):
```
docker compose up --scale presto-worker=3
```

# Use cli:

After all containers are in "Started" states (check by `docker ps`):

```bash
# On your host
docker exec -it compose-presto-coordinator-1 sh

# In presto-coordinator container
/opt/presto-cli --catalog clp --schema default --server localhost:8080
```

Example query:
```sql
SELECT * FROM default LIMIT 1;
```

# Delete docker Cluster

```bash
docker compose down
```



[clp-json-v0.4.0]: https://github.com/y-scope/clp/releases/tag/v0.4.0
[docker]: https://docs.docker.com/engine/install
[postgresql dataset]: https://zenodo.org/records/10516402

