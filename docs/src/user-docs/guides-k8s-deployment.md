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

* [`kubectl`][kubectl] >= 1.30
* [Helm] >= 4.0
* A Kubernetes cluster (see [Setting up a cluster](#setting-up-a-cluster) below)
* When not using S3 storage, a shared filesystem accessible by all worker pods (e.g., NFS,
  [SeaweedFS]) or local storage for single-node deployments

---

## Setting up a cluster

You can deploy CLP on either a local development cluster or a production Kubernetes cluster.

### Option 1: Local development with `kind`

[`kind`][kind] (Kubernetes in Docker) is ideal for testing and development. It runs a Kubernetes
cluster inside Docker containers on your local machine.

For single-host `kind` deployments, see the [quick-start guides][quick-start], which cover creating
a `kind` cluster and installing the Helm chart.

### Option 2: Production Kubernetes cluster

For production deployments, you can use any Kubernetes distribution:

* Managed Kubernetes services: [Amazon EKS][eks], [Azure AKS][aks], [Google GKE][gke]
* Self-hosted: [`kubeadm`][kubeadm], [k3s], [RKE2]

#### Setting up a cluster with `kubeadm`

`kubeadm` is the official Kubernetes tool for bootstrapping clusters. You can follow the
[official `kubeadm` installation guide][kubeadm] to install the prerequisites, container runtime,
and `kubeadm` on all nodes. Then follow the steps below to create a cluster.

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

   Run the `kubeadm join` command you saved from step 1. It should look something like:

   ```bash
   sudo kubeadm join <control-plane-ip>:6443 \
     --token <token> \
     --discovery-token-ca-cert-hash sha256:<hash>
   ```

   If you need to regenerate the command, on the control-plane node, run:

   ```bash
   kubeadm token create --print-join-command
   ```

---

## Installing the Helm chart

Once your cluster is ready, you can install CLP using the Helm chart.

### Getting the chart

The CLP Helm chart is published to a [Helm repository][clp-helm-repo] hosted on GitHub Pages.

```bash
helm repo add clp https://y-scope.github.io/clp
helm repo update clp
```

#### Production cluster requirements (optional)

The following configurations are optional but recommended for production deployments. You can skip
this section for testing or development.

1. **Shared storage for workers** (required for multi-node clusters using filesystem storage):

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
   [SeaweedFS section][seaweedfs-setup] in the Docker Compose deployment guide for setup
   instructions.

2. **External databases** (recommended for production):
   * See the [external database setup guide][external-db-guide] for using external
     MariaDB/MySQL and MongoDB databases

### Basic installation

Generate credentials and install CLP:

```bash
# Credentials (change these for production)
export CLP_DB_PASS="pass"
export CLP_DB_ROOT_PASS="root-pass"
export CLP_QUEUE_PASS="pass"
export CLP_REDIS_PASS="pass"

# Worker replicas (increase for multi-node clusters)
export CLP_COMPRESSION_WORKER_REPLICAS=1
export CLP_QUERY_WORKER_REPLICAS=1
export CLP_REDUCER_REPLICAS=1

helm install clp clp/clp DOCS_VAR_HELM_VERSION_FLAG \
  --set credentials.database.password="$CLP_DB_PASS" \
  --set credentials.database.root_password="$CLP_DB_ROOT_PASS" \
  --set credentials.queue.password="$CLP_QUEUE_PASS" \
  --set credentials.redis.password="$CLP_REDIS_PASS" \
  --set compressionWorker.replicas="$CLP_COMPRESSION_WORKER_REPLICAS" \
  --set queryWorker.replicas="$CLP_QUERY_WORKER_REPLICAS" \
  --set reducer.replicas="$CLP_REDUCER_REPLICAS"
```

### Multi-node deployment

For multi-node clusters with shared storage mounted on all nodes (e.g., NFS/CephFS via
`/etc/fstab`), enable distributed storage mode and configure multiple worker replicas:

```bash
helm install clp clp/clp DOCS_VAR_HELM_VERSION_FLAG \
  --set distributedDeployment=true \
  --set compressionWorker.replicas=3 \
  --set queryWorker.replicas=3 \
  --set reducer.replicas=3
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
    retention_period: 43200  # (in minutes) 30 days

  # Enable MCP server
  mcp_server:
    port: 30800
    logging_level: "INFO"

  # Configure results cache
  results_cache:
    retention_period: 120  # (in minutes) 2 hours

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
helm install clp clp/clp DOCS_VAR_HELM_VERSION_FLAG -f custom-values.yaml
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

To run compression workers, query workers, and reducers in separate node pools:

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

   distributedDeployment: true

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

   reducer:
     replicas: 2
     scheduling:
       nodeSelector:
         yscope.io/nodeType: query
   ```

3. Install:

   ```bash
   helm install clp clp/clp DOCS_VAR_HELM_VERSION_FLAG -f dedicated-scheduling.yaml
   ```

#### Shared node pool

To run all worker types in the same node pool:

1. Label your nodes:

   ```bash
   kubectl label nodes node1 node2 node3 node4 yscope.io/nodeType=compute
   ```

2. Configure scheduling:

   ```{code-block} yaml
   :caption: shared-scheduling.yaml

   distributedDeployment: true

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

   reducer:
     replicas: 2
     scheduling:
       nodeSelector:
         yscope.io/nodeType: compute
   ```

3. Install:

   ```bash
   helm install clp clp/clp DOCS_VAR_HELM_VERSION_FLAG -f shared-scheduling.yaml
   ```

---

## Verifying the deployment

After installing the Helm chart, you can verify that all components are running correctly as
follows.

### Check pod status

Wait for all pods to be ready:

```bash
# Watch pod status
kubectl get pods -w

# Wait for all pods to be ready
kubectl wait pods --all --for=condition=Ready --timeout=300s
```

The output should show all pods are in the `Running` state:

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

CLP runs initialization jobs on first deployment. Check that these jobs completed successfully:

```bash
# Check job completion
kubectl get jobs

# Expected output:
# NAME                              COMPLETIONS   DURATION   AGE
# clp-db-table-creator              1/1           5s         2m
# clp-results-cache-indices-creator 1/1           3s         2m
```

### Access the Web UI

Once all pods are ready, you access the CLP Web UI at: `http://<node-ip>:30000` (the value of
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
[release][clp-releases] matching the chart's `appVersion`, then update the following configurations
in `etc/clp-config.yaml`:

```yaml
database:
  port: 30306  # Match `clpConfig.database.port` in Helm values
results_cache:
  port: 30017  # Match `clpConfig.results_cache.port` in Helm values
```

Alternatively, use the Web UI ([clp-json][webui-clp-json] or [clp-text][webui-clp-text]) to compress
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
# For debugging the published chart from the repository
helm install clp clp/clp DOCS_VAR_HELM_VERSION_FLAG --dry-run --debug

# For debugging local chart changes during development
helm install clp /path/to/local/chart --dry-run --debug
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
Uninstalling the Helm release will delete all CLP pods and services. However, dynamically
provisioned PersistentVolumeClaims (database, results cache, archives, streams) may be retained
depending on the cluster's `reclaimPolicy`. To completely remove all data, delete the PVCs manually.
:::

---

## Cleaning up

To tear down a `kubeadm` cluster:

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
[clp-helm-repo]: https://y-scope.github.io/clp
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
[quick-start]: quick-start/index.md
[retention-guide]: guides-retention.md
[rfc-1918]: https://datatracker.ietf.org/doc/html/rfc1918#section-3
[RKE2]: https://docs.rke2.io/
[s3-storage]: guides-using-object-storage/index
[SeaweedFS]: https://github.com/seaweedfs/seaweedfs
[seaweedfs-setup]: guides-docker-compose-deployment.md#setting-up-seaweedfs
[webui-clp-json]: quick-start/clp-json.md#searching-from-the-ui
[webui-clp-text]: quick-start/clp-text.md#searching-from-the-ui
