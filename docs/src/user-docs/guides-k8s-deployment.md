# Kubernetes deployment

This guide explains how to deploy CLP on Kubernetes using [Helm]. This provides an alternative to
Docker Compose and enables deployment on Kubernetes clusters ranging from local development setups
to production environments.

:::{note}
For a detailed overview of CLP's services and their dependencies, see the
[deployment orchestration design doc][design-orchestration].
:::

---

## Requirements

The following tools are required to deploy CLP on Kubernetes:

* [kubectl] >= 1.30
* [Helm] >= 4.0
* A Kubernetes cluster (see [Setting up a cluster](#setting-up-a-cluster) below)
* When not using S3 storage, a shared filesystem accessible by all worker pods (e.g., NFS,
  [SeaweedFS]) or local storage for single-node deployments

---

## Setting up a cluster

You can deploy CLP on either a local development cluster or a production Kubernetes cluster.

### Option 1: Local development with kind

[kind] (Kubernetes in Docker) is ideal for testing and development. It runs a Kubernetes cluster
inside Docker containers on your local machine.

For single-host kind deployments, see the [quick-start guides][quick-start], which cover creating a
kind cluster and installing the Helm chart.

### Option 2: Production Kubernetes cluster

For production deployments, you can use any Kubernetes distribution:

* Managed Kubernetes services: [Amazon EKS][eks], [Google GKE][gke], [Azure AKS][aks]
* Self-hosted: [kubeadm], [k3s], [RKE2]

#### Setting up a cluster with kubeadm

[kubeadm] is the official Kubernetes tool for bootstrapping clusters. Follow the
[official kubeadm installation guide][kubeadm] to install the prerequisites, container runtime,
and kubeadm on all nodes.

1. **Initialize the control plane** (on the control-plane node only):

   ```bash
   sudo kubeadm init --pod-network-cidr=10.244.0.0/16
   ```

   :::{tip}
   Save the `kubeadm join` command printed at the end of the output. You'll need it to join worker
   nodes later.
   :::

   :::{note}
   The `--pod-network-cidr` specifies the IP range for pods. If `10.244.0.0/16` conflicts with your
   network, use a different private range as [RFC 1918][rfc-1918] specifies (e.g., `192.168.0.0/16`,
   `172.16.0.0/16`, or `10.200.0.0/16`).
   :::

   To set up `kubectl` for your user:

   ```bash
   mkdir -p "$HOME/.kube"
   sudo cp -i /etc/kubernetes/admin.conf "$HOME/.kube/config"
   sudo chown "$(id -u):$(id -g)" "$HOME/.kube/config"
   ```

2. **Install a CNI plugin** (on the control-plane node):

   A CNI plugin is required for pod-to-pod networking. The following installs [Cilium], a
   high-performance CNI that uses eBPF:

   ```bash
   helm repo add cilium https://helm.cilium.io/
   helm repo update
   helm install cilium cilium/cilium --namespace kube-system \
     --set ipam.operator.clusterPoolIPv4PodCIDRList=10.244.0.0/16
   ```

   :::{note}
   The `clusterPoolIPv4PodCIDRList` must match the `--pod-network-cidr` used in `kubeadm init`.
   :::

3. **Join worker nodes** (on each worker node):

   Run `kubeadm join` with the token and hash you saved from step 1:

   ```bash
   sudo kubeadm join <control-plane-ip>:6443 \
     --token <token> \
     --discovery-token-ca-cert-hash sha256:<hash>
   ```

   If you lost the command, regenerate it on the control-plane node with
   `kubeadm token create --print-join-command`.

---

## Installing the Helm chart

Once your cluster is ready, you can install CLP using the Helm chart.

### Getting the chart

The CLP Helm chart is located in the repository at `tools/deployment/package-helm/`.

```bash
# Clone the repository (if you haven't already)
git clone https://github.com/y-scope/clp.git
cd clp/tools/deployment/package-helm
```

#### Production cluster requirements (optional)

The following configurations are optional but recommended for production deployments. You can skip
this section for testing or development.

1. **Storage for CLP package services' data and logs** (optional, for centralized debugging):

   The Helm chart creates static PersistentVolumes using local host paths by default, so no
   StorageClass configuration is required for basic deployments. For easier debugging, you can
   configure a centralized storage backend for the following directories:

   * `data_directory` - where CLP stores runtime data
   * `logs_directory` - where CLP services write logs
   * `tmp_directory` - where temporary files are stored

   :::{note}
   We aim to improve the logging infrastructure so mapping log volumes will not be required in the
   future. See [issue #1760][logging-infra-issue] for details.
   :::

2. **Shared storage for workers** (required for multi-node clusters using filesystem storage):

   :::{tip}
   [S3 storage][s3-storage] is **strongly recommended** for multi-node clusters as it does not
   require shared local storage between workers. If you use S3 storage, you can skip this section.
   :::

   For multi-node clusters using filesystem storage, the following directories **must** be
   accessible from all worker nodes at the same paths. Without shared storage, compressed logs
   created by one worker cannot be searched by other workers.

   * `archive_output.storage.directory` - where compressed archives are stored
   * `stream_output.storage.directory` - where stream files are stored
   * `logs_input.directory` - where input logs are read from

   Set up NFS, SeaweedFS, or another shared filesystem to provide this access. See the
   [multi-host deployment guide][multi-host-guide] for SeaweedFS setup instructions.

3. **External databases** (recommended for production):
   * See the [external database setup guide][external-db-guide] for using external
     MariaDB/MySQL and MongoDB databases

### Basic installation

Create the required directories on all worker nodes:

```bash
export CLP_HOME="/tmp/clp"

mkdir -p "$CLP_HOME/var/data/"{archives,streams,staged-archives,staged-streams} \
         "$CLP_HOME/var/log/"{compression_scheduler,compression_worker,user} \
         "$CLP_HOME/var/log/"{query_scheduler,query_worker,reducer} \
         "$CLP_HOME/var/tmp"
```

Then on the **control-plane node**, generate credentials and install CLP:

```bash
export CLP_HOME="/tmp/clp"

mkdir -p "$CLP_HOME/var/"{data,log}/{database,queue,redis,results_cache} \
         "$CLP_HOME/var/data/"{archives,streams,staged-archives,staged-streams} \
         "$CLP_HOME/var/log/"{compression_scheduler,compression_worker,user} \
         "$CLP_HOME/var/log/"{query_scheduler,query_worker,reducer} \
         "$CLP_HOME/var/log/"{garbage_collector,api_server,log_ingestor,mcp_server} \
         "$CLP_HOME/var/tmp"

# Credentials (change these for production)
export CLP_DB_PASS="pass"
export CLP_DB_ROOT_PASS="root-pass"
export CLP_QUEUE_PASS="pass"
export CLP_REDIS_PASS="pass"

# Worker replicas (increase for multi-node clusters)
export CLP_COMPRESSION_WORKER_REPLICAS=1
export CLP_QUERY_WORKER_REPLICAS=1

helm install clp . \
  --set clpConfig.data_directory="$CLP_HOME/var/data" \
  --set clpConfig.logs_directory="$CLP_HOME/var/log" \
  --set clpConfig.tmp_directory="$CLP_HOME/var/tmp" \
  --set clpConfig.archive_output.storage.directory="$CLP_HOME/var/data/archives" \
  --set clpConfig.stream_output.storage.directory="$CLP_HOME/var/data/streams" \
  --set credentials.database.password="$CLP_DB_PASS" \
  --set credentials.database.root_password="$CLP_DB_ROOT_PASS" \
  --set credentials.queue.password="$CLP_QUEUE_PASS" \
  --set credentials.redis.password="$CLP_REDIS_PASS" \
  --set compressionWorker.replicas="$CLP_COMPRESSION_WORKER_REPLICAS" \
  --set queryWorker.replicas="$CLP_QUERY_WORKER_REPLICAS"
```

### Multi-node deployment

For multi-node clusters with shared storage mounted on all nodes (e.g., NFS/CephFS via
`/etc/fstab`), enable distributed storage mode and configure multiple worker replicas:

```bash
helm install clp . \
  --set distributedDeployment=true \
  --set compressionWorker.replicas=3 \
  --set queryWorker.replicas=3
```

### Installation with custom values

For highly customized deployments, create a values file instead of using many `--set` flags:

```{code-block} yaml
:caption: custom-values.yaml

# Use a custom image. For local images, import to each node's container runtime first.
image:
  clpPackage:
    repository: "clp-package"
    pullPolicy: "Never"  # Use "Never" for local images, "IfNotPresent" for remote
    tag: "latest"

# Adjust worker concurrency
workerConcurrency: 16

# Configure CLP settings
clpConfig:
  # Use clp-text, instead of clp-json (default)
  package:
    storage_engine: "clp"  # Use "clp-s" for clp-json, "clp" for clp-text
    query_engine: "clp"   # Use "clp-s" for clp-json, "clp" for clp-text, "presto" for Presto

  # Configure archive output
  archive_output:
    target_archive_size: 536870912  # 512 MB
    compression_level: 6

  # Enable MCP server
  mcp_server:
    port: 30800
    logging_level: "INFO"

  # Configure data retention (in minutes)
  archive_output:
    retention_period: 10080  # 7 days
  results_cache:
    retention_period: 120  # 2 hours

# Override credentials (use secrets in production!)
credentials:
  database:
    username: "clp-user"
    password: "your-db-password"
    root_username: "root"
    root_password: "your-db-root-password"
  queue:
    username: "clp-user"
    password: "your-queue-password"
  redis:
    password: "your-redis-password"
```

Install with custom values:

```bash
helm install clp . -f custom-values.yaml
```

::::{tip}
To preview the generated Kubernetes manifests before installing, use `helm template`:

```bash
helm template clp . -f custom-values.yaml
```

::::

### Worker scheduling

You can control where workers are scheduled using standard Kubernetes scheduling primitives
(`nodeSelector`, `affinity`, `tolerations`, `topologySpreadConstraints`).

#### Dedicated node pools

To run compression and query workers on separate node pools:

1. Label your nodes:

   ```bash
   # Label compression nodes
   kubectl label nodes node1 node2 yscope.io/nodeType=compression

   # Label query nodes
   kubectl label nodes node3 node4 yscope.io/nodeType=query
   ```

2. Configure scheduling:

   ```{code-block} yaml
   :caption: dedicated-scheduling.yaml

   compressionWorker:
     replicas: 2
     scheduling:
       nodeSelector:
         yscope.io/nodeType: compression

   queryWorker:
     replicas: 2
     scheduling:
       nodeSelector:
         yscope.io/nodeType: query
   ```

3. Install:

   ```bash
   helm install clp . -f dedicated-scheduling.yaml --set distributedDeployment=true
   ```

#### Shared node pool

To run both worker types on the same node pool:

1. Label your nodes:

   ```bash
   kubectl label nodes node1 node2 node3 node4 yscope.io/nodeType=compute
   ```

2. Configure scheduling:

   ```{code-block} yaml
   :caption: shared-scheduling.yaml

   compressionWorker:
     replicas: 2
     scheduling:
       nodeSelector:
         yscope.io/nodeType: compute
       topologySpreadConstraints:
         - maxSkew: 1
           topologyKey: "kubernetes.io/hostname"
           whenUnsatisfiable: "DoNotSchedule"
           labelSelector:
             matchLabels:
               app.kubernetes.io/component: compression-worker

   queryWorker:
     replicas: 2
     scheduling:
       nodeSelector:
         yscope.io/nodeType: compute
   ```

3. Install:

   ```bash
   helm install clp . -f shared-scheduling.yaml --set distributedDeployment=true
   ```

### Common configuration options

The following table lists commonly used Helm values. For a complete list, see `values.yaml` in the
chart directory.

| Parameter                                    | Description                                    | Default                           |
|----------------------------------------------|------------------------------------------------|-----------------------------------|
| `image.clpPackage.repository`                | CLP package image repository                   | `ghcr.io/y-scope/clp/clp-package` |
| `image.clpPackage.tag`                       | Image tag                                      | `main`                            |
| `workerConcurrency`                          | Number of worker processes                     | `8`                               |
| `distributedDeployment`                      | Distributed/multi-node deployment mode         | `false`                           |
| `compressionWorker.replicas`                 | Number of compression worker replicas          | `1`                               |
| `compressionWorker.scheduling`               | Scheduling config for compression workers      | `{}`                              |
| `queryWorker.replicas`                       | Number of query worker replicas                | `1`                               |
| `queryWorker.scheduling`                     | Scheduling config for query workers            | `{}`                              |
| `storage.storageClassName`                   | StorageClass name (created if "local-storage") | `local-storage`                   |
| `allowHostAccessForSbinScripts`              | Expose database/cache for sbin scripts         | `true`                            |
| `clpConfig.package.storage_engine`           | Storage engine (`clp-s` or `clp`)              | `clp-s`                           |
| `clpConfig.package.query_engine`             | Query engine (`clp-s`, `clp`, or `presto`)     | `clp-s`                           |
| `clpConfig.webui.port`                       | Web UI NodePort                                | `30000`                           |
| `clpConfig.api_server.port`                  | API server NodePort                            | `30301`                           |
| `clpConfig.database.port`                    | Database NodePort                              | `30306`                           |
| `clpConfig.results_cache.port`               | Results cache (MongoDB) NodePort               | `30017`                           |
| `clpConfig.mcp_server.port`                  | MCP server NodePort                            | `30800`                           |
| `clpConfig.logs_input.directory`             | Directory containing logs to compress          | `/`                               |
| `clpConfig.data_directory`                   | Directory for data storage                     | `/tmp/clp/var/data`               |
| `clpConfig.logs_directory`                   | Directory for log files                        | `/tmp/clp/var/log`                |
| `clpConfig.tmp_directory`                    | Directory for temporary files                  | `/tmp/clp/var/tmp`                |
| `clpConfig.archive_output.storage.directory` | Directory for compressed archives              | `/tmp/clp/var/data/archives`      |
| `clpConfig.stream_output.storage.directory`  | Directory for stream files                     | `/tmp/clp/var/data/streams`       |
| `clpConfig.archive_output.retention_period`  | Archive retention (minutes, null to disable)   | `null`                            |
| `clpConfig.results_cache.retention_period`   | Search results retention (minutes)             | `60`                              |

---

## Verifying the deployment

After installing the Helm chart, verify that all components are running correctly.

### Check pod status

Wait for all pods to be ready:

```bash
# Watch pod status
kubectl get pods -w

# Wait for all pods to be ready
kubectl wait pods --all --for=condition=Ready --timeout=300s
```

Expected output shows all pods in `Running` state:

```text
NAME                                        READY   STATUS    RESTARTS   AGE
clp-api-server-...                          1/1     Running   0          2m
clp-compression-scheduler-...               1/1     Running   0          2m
clp-compression-worker-...                  1/1     Running   0          2m
clp-database-0                              1/1     Running   0          2m
clp-garbage-collector-...                   1/1     Running   0          2m
clp-query-scheduler-...                     1/1     Running   0          2m
clp-query-worker-...                        1/1     Running   0          2m
clp-queue-0                                 1/1     Running   0          2m
clp-reducer-...                             1/1     Running   0          2m
clp-redis-0                                 1/1     Running   0          2m
clp-results-cache-0                         1/1     Running   0          2m
clp-webui-...                               1/1     Running   0          2m
```

### Check initialization jobs

CLP runs initialization jobs on first deployment:

```bash
# Check job completion
kubectl get jobs

# Expected output:
# NAME                              COMPLETIONS   DURATION   AGE
# clp-db-table-creator              1/1           5s         2m
# clp-results-cache-indices-creator 1/1           3s         2m
```

### Access the Web UI

Once all pods are ready, access the CLP Web UI: `http://<node-ip>:30000` (the value of
`clpConfig.webui.port`)

---

## Using CLP

With CLP deployed on Kubernetes, you can compress and search logs using the same workflows as
Docker Compose deployments. Refer to the quick-start guide for your chosen flavor:

::::{grid} 1 1 2 2
:gutter: 2

:::{grid-item-card}
:link: quick-start/clp-json
Using clp-json
^^^
How to compress and search JSON logs.
:::

:::{grid-item-card}
:link: quick-start/clp-text
Using clp-text
^^^
How to compress and search unstructured text logs.
:::
::::

:::{note}
By default (`allowHostAccessForSbinScripts: true`), the database and results cache are exposed on
NodePorts, allowing you to use `sbin/` scripts from the CLP package. Download a
[release][clp-releases] matching the chart's `appVersion`, then configure `etc/clp-config.yaml`:

```yaml
database:
  port: 30306  # Match `clpConfig.database.port` in Helm values
results_cache:
  port: 30017  # Match `clpConfig.results_cache.port` in Helm values
```

Alternatively, use the Web UI ([clp-json][webui-clp-json]|[clp-text][webui-clp-text]) to compress
logs and search interactively, or the [API server][api-server] to submit queries and view results
programmatically.
:::

---

## Monitoring and debugging

To check the status of pods:

```bash
kubectl get pods
```

To view logs for a specific pod:

```bash
kubectl logs -f <pod-name>
```

To execute commands in a pod:

```bash
kubectl exec -it <pod-name> -- /bin/bash
```

To debug Helm chart issues:

```bash
helm install clp . --dry-run --debug
```

---

## Managing releases

This section covers how to manage your CLP Helm release.

:::{note}
Upgrade and rollback are not yet supported. We plan to add support as we finalize the migration
mechanism.
:::

### Uninstall CLP

```bash
helm uninstall clp
```

:::{warning}
Uninstalling the Helm release will delete all CLP pods and services. However, PersistentVolumes
with `Retain` policy will preserve your data. To completely remove all data, delete the PVs and
the data directories manually.
:::

---

## Cleaning up

To tear down a kubeadm cluster:

1. **Uninstall Cilium** (on the control-plane):

   ```bash
   helm uninstall cilium --namespace kube-system
   ```

2. **Reset each node** (run on all worker nodes first, then the control-plane):

   ```bash
   sudo kubeadm reset -f
   sudo rm -rf /etc/cni/net.d/*
   sudo umount /var/run/cilium/cgroupv2/
   sudo rm -rf /var/run/cilium
   ```

3. **Clean up kubeconfig** (on the control-plane):

   ```bash
   rm -rf ~/.kube
   ```

---

## Related guides

* [Docker Compose deployment][docker-compose-deployment]: Docker Compose orchestration for single or
  multi-host setups
* [External database setup][external-db-guide]: Using external MariaDB and MongoDB
* [Using object storage][s3-storage]: Configuring S3 storage
* [Configuring retention periods][retention-guide]: Setting up data retention policies

[aks]: https://azure.microsoft.com/en-us/products/kubernetes-service
[api-server]: guides-using-the-api-server.md
[Cilium]: https://cilium.io/
[clp-releases]: https://github.com/y-scope/clp/releases
[design-orchestration]: ../dev-docs/design-deployment-orchestration.md
[docker-compose-deployment]: guides-docker-compose-deployment.md
[eks]: https://aws.amazon.com/eks/
[external-db-guide]: guides-external-database.md
[gke]: https://cloud.google.com/kubernetes-engine
[Helm]: https://helm.sh/
[k3s]: https://k3s.io/
[kind]: https://kind.sigs.k8s.io/
[kubeadm]: https://kubernetes.io/docs/setup/production-environment/tools/kubeadm/install-kubeadm/
[kubectl]: https://kubernetes.io/docs/tasks/tools/
[logging-infra-issue]: https://github.com/y-scope/clp/issues/1760
[multi-host-guide]: guides-docker-compose-deployment.md
[quick-start]: quick-start/index.md
[retention-guide]: guides-retention.md
[rfc-1918]: https://datatracker.ietf.org/doc/html/rfc1918#section-3
[RKE2]: https://docs.rke2.io/
[s3-storage]: guides-using-object-storage/index
[SeaweedFS]: https://github.com/seaweedfs/seaweedfs
[webui-clp-json]: quick-start/clp-json.md#searching-from-the-ui
[webui-clp-text]: quick-start/clp-text.md#searching-from-the-ui
