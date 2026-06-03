{{/*
Expands the name of the chart.

@return {string} The chart name (truncated to 63 characters)
*/}}
{{- define "clp.name" -}}
{{- default .Chart.Name .Values.nameOverride | trunc 63 | trimSuffix "-" }}
{{- end }}

{{/*
Creates a default fully qualified app name. We truncate at 63 chars because some Kubernetes name
fields are limited to this (by the DNS naming spec). If release name contains chart name it will be
used as a full name.

@return {string} The fully qualified app name (truncated to 63 characters)
*/}}
{{- define "clp.fullname" -}}
{{- if .Values.fullnameOverride }}
{{- .Values.fullnameOverride | trunc 63 | trimSuffix "-" }}
{{- else }}
{{- $name := default .Chart.Name .Values.nameOverride }}
{{- if contains $name .Release.Name }}
{{- .Release.Name | trunc 63 | trimSuffix "-" }}
{{- else }}
{{- printf "%s-%s" .Release.Name $name | trunc 63 | trimSuffix "-" }}
{{- end }}
{{- end }}
{{- end }}

{{/*
Creates chart name and version as used by the chart label.

@return {string} Chart name and version (truncated to 63 characters)
*/}}
{{- define "clp.chart" -}}
{{- printf "%s-%s" .Chart.Name .Chart.Version | replace "+" "_" | trunc 63 | trimSuffix "-" }}
{{- end }}

{{/*
Creates common labels for all resources.

@return {string} YAML-formatted common labels
*/}}
{{- define "clp.labels" -}}
helm.sh/chart: {{ include "clp.chart" . }}
{{ include "clp.selectorLabels" . }}
{{- if .Chart.AppVersion }}
app.kubernetes.io/version: {{ .Chart.AppVersion | quote }}
{{- end }}
app.kubernetes.io/managed-by: {{ .Release.Service }}
{{- end }}

{{/*
Gets or generates the instance ID.
It ensures the exact same UUID is used across all templates during a single helm install/upgrade.
*/}}
{{- define "clp.instanceId" -}}
{{- if not .Values.global }}
  {{- $_ := set .Values "global" dict -}}
{{- end }}
{{- if not .Values.global.instanceId }}
  {{- $existingName := printf "%s-instance-id" (include "clp.fullname" .) }}
  {{- $existing := lookup "v1" "ConfigMap" .Release.Namespace $existingName }}
  {{- if $existing }}
    {{- $_ := set .Values.global "instanceId" (index $existing.data "instanceId") -}}
  {{- else }}
    {{- $_ := set .Values.global "instanceId" uuidv4 -}}
  {{- end }}
{{- end }}
{{- .Values.global.instanceId -}}
{{- end -}}

{{/*
Creates common CLP resource attributes for telemetry.

@return {string} Comma-separated OpenTelemetry resource attributes
*/}}
{{- define "clp.resourceAttributes" -}}
{{- printf
  "clp.deployment.id=%s,service.version=%s,clp.deployment.method=helm,clp.storage.engine=%s"
  (include "clp.instanceId" .)
  .Chart.AppVersion
  .Values.clpConfig.package.storage_engine -}}
{{- end -}}

{{/*
Generates the OTLP JSON payload for topology metrics emission.

This produces the same payload structure as DockerComposeController._emit_topology_metrics()
in controller.py, ensuring feature parity between Docker Compose and Helm deployments.

@return {string} OTLP JSON payload for topology metrics
*/}}
{{- define "clp.topologyMetricsPayload" -}}
{{- $timestampNs := now.UnixNano -}}
{{- $compressionWorkerScheduling := .Values.scheduling.compressionWorker -}}
{{- $compressionWorkerReplicas := $compressionWorkerScheduling.replicas | default 1 | int -}}
{{- $queryWorkerReplicas := .Values.scheduling.queryWorker.replicas | default 1 | int -}}
{{- $reducerReplicas := .Values.scheduling.reducer.replicas | default 1 | int -}}
{{- $workerConcurrency := .Values.workerConcurrency | default 8 | int -}}
{{- printf `{"resourceMetrics":[{"resource":{"attributes":[{"key":"service.name","value":{"stringValue":"controller"}}]},"scopeMetrics":[{"scope":{"name":"clp.controller"},"metrics":[{"name":"clp.deployment.compression_worker_replicas","gauge":{"dataPoints":[{"asInt":"%d","timeUnixNano":"%d"}]}},{"name":"clp.deployment.compression_worker_concurrency","gauge":{"dataPoints":[{"asInt":"%d","timeUnixNano":"%d"}]}},{"name":"clp.deployment.query_worker_replicas","gauge":{"dataPoints":[{"asInt":"%d","timeUnixNano":"%d"}]}},{"name":"clp.deployment.query_worker_concurrency","gauge":{"dataPoints":[{"asInt":"%d","timeUnixNano":"%d"}]}},{"name":"clp.deployment.reducer_replicas","gauge":{"dataPoints":[{"asInt":"%d","timeUnixNano":"%d"}]}},{"name":"clp.deployment.reducer_concurrency","gauge":{"dataPoints":[{"asInt":"%d","timeUnixNano":"%d"}]}}]}]}]}` $compressionWorkerReplicas $timestampNs $workerConcurrency $timestampNs $queryWorkerReplicas $timestampNs $workerConcurrency $timestampNs $reducerReplicas $timestampNs $workerConcurrency $timestampNs -}}
{{- end -}}

{{/*
Provides environment variables for telemetry (except service.name).
*/}}
{{- define "clp.telemetryEnv" -}}
{{- if .Values.clpConfig.telemetry.disable }}
- name: "CLP_DISABLE_TELEMETRY"
  value: "true"
{{- else }}
- name: "OTEL_EXPORTER_OTLP_ENDPOINT"
  value: {{ include "clp.otelCollectorUrl" . | quote }}
{{- end }}
{{- end -}}

{{/*
Creates selector labels for matching resources.

@return {string} YAML-formatted selector labels
*/}}
{{- define "clp.selectorLabels" -}}
app.kubernetes.io/name: {{ include "clp.name" . }}
app.kubernetes.io/instance: {{ .Release.Name }}
{{- end }}

{{/*
Creates an image reference for a component.

For components with variants (e.g., database with mariadb/mysql), pass the "variant" parameter.
If no variant is provided, the component is looked up directly under .Values.image.

When a "digest" field is present in the image config, the reference uses the format
repository@digest. Otherwise, it uses repository:tag.

For the clpPackage component, if no tag is specified, the chart's appVersion is used as a default.
For all other components, the tag must be specified in values.yaml.

@param {object} root Root template context (required)
@param {string} component Key under .Values.image (e.g., "clpPackage", "redis", "database")
@param {string} [variant] Sub-key for components with variants (e.g., "mariadb" or "mysql" for "database")
@return {string} Full image reference (repository@digest or repository:tag)
*/}}
{{- define "clp.image.ref" -}}
{{- $img := index .root.Values.image .component -}}
{{- if hasKey . "variant" -}}
  {{- $img = index $img .variant -}}
{{- end -}}
{{- if $img.digest -}}
{{- printf "%s@%s" $img.repository $img.digest -}}
{{- else -}}
{{- $tag := $img.tag -}}
{{- if not $tag -}}
  {{- if eq .component "clpPackage" -}}
    {{- $tag = .root.Chart.AppVersion -}}
  {{- else -}}
    {{- fail (printf "image.%s.tag is required" .component) -}}
  {{- end -}}
{{- end -}}
{{- printf "%s:%s" $img.repository $tag -}}
{{- end -}}
{{- end }}

{{/*
Creates timings for readiness probes (faster checks for quicker startup).

@return {string} YAML-formatted readiness probe timing configuration
*/}}
{{- define "clp.readinessProbeTimings" -}}
initialDelaySeconds: 6
periodSeconds: 2
timeoutSeconds: 2
failureThreshold: 10
{{- end }}

{{/*
Creates timings for liveness probes.

@return {string} YAML-formatted liveness probe timing configuration
*/}}
{{- define "clp.livenessProbeTimings" -}}
initialDelaySeconds: 180
periodSeconds: 30
timeoutSeconds: 4
failureThreshold: 3
{{- end }}

{{/*
Creates a volume name for persistent storage resources.

Used for:
- Pod volume names (standalone)
- PV/PVC resource names (combined with fullname)
- StatefulSet volumeClaimTemplate names

@param {string} component_category (e.g., "database", "shared-data")
@param {string} name (e.g., "archives", "data", "logs")
@return {string} Volume name in the format "{component_category}-{name}"
*/}}
{{- define "clp.volumeName" -}}
{{- printf "%s-%s" .component_category .name -}}
{{- end }}

{{/*
Creates a PersistentVolumeClaim for the given component.

Uses the cluster's default StorageClass for dynamic provisioning.

@param {object} root Root template context
@param {string} component_category (e.g., "database", "shared-data")
@param {string} name (e.g., "archives", "data", "logs")
@param {string} capacity Storage capacity
@param {string[]} accessModes Access modes
@return {string} YAML-formatted PersistentVolumeClaim resource
*/}}
{{- define "clp.createPvc" -}}
apiVersion: "v1"
kind: "PersistentVolumeClaim"
metadata:
  name: {{ include "clp.fullname" .root }}-{{ include "clp.volumeName" . }}
  labels:
    {{- include "clp.labels" .root | nindent 4 }}
    app.kubernetes.io/component: {{ .component_category | quote }}
spec:
  accessModes: {{ .accessModes }}
  resources:
    requests:
      storage: {{ .capacity }}
{{- end }}

{{/*
Creates a volume definition that references a PersistentVolumeClaim.

@param {object} root Root template context
@param {string} component_category (e.g., "database", "shared-data")
@param {string} name (e.g., "archives", "data", "logs")
@return {string} YAML-formatted volume definition
*/}}
{{- define "clp.pvcVolume" -}}
name: {{ include "clp.volumeName" . | quote }}
persistentVolumeClaim:
  claimName: {{ include "clp.fullname" .root }}-{{ include "clp.volumeName" . }}
{{- end }}

{{/*
Gets the host for the database service.

@param {object} . Root template context
@return {string} The database host
*/}}
{{- define "clp.databaseHost" -}}
{{- if has "database" .Values.clpConfig.bundled -}}
{{- printf "%s-database" (include "clp.fullname" .) -}}
{{- else -}}
{{- .Values.clpConfig.database.host -}}
{{- end -}}
{{- end }}

{{/*
Gets the port for the database service.

@param {object} . Root template context
@return {string} The database port
*/}}
{{- define "clp.databasePort" -}}
{{- if has "database" .Values.clpConfig.bundled -}}
3306
{{- else -}}
{{- .Values.clpConfig.database.port -}}
{{- end -}}
{{- end }}

{{/*
Gets the host for the queue service.

@param {object} . Root template context
@return {string} The queue host
*/}}
{{- define "clp.queueHost" -}}
{{- if has "queue" .Values.clpConfig.bundled -}}
{{- printf "%s-queue" (include "clp.fullname" .) -}}
{{- else -}}
{{- .Values.clpConfig.queue.host -}}
{{- end -}}
{{- end }}

{{/*
Gets the port for the queue service.

@param {object} . Root template context
@return {string} The queue port
*/}}
{{- define "clp.queuePort" -}}
{{- if has "queue" .Values.clpConfig.bundled -}}
5672
{{- else -}}
{{- .Values.clpConfig.queue.port -}}
{{- end -}}
{{- end }}

{{/*
Gets the host for the Redis service.

@param {object} . Root template context
@return {string} The Redis host
*/}}
{{- define "clp.redisHost" -}}
{{- if has "redis" .Values.clpConfig.bundled -}}
{{- printf "%s-redis" (include "clp.fullname" .) -}}
{{- else -}}
{{- .Values.clpConfig.redis.host -}}
{{- end -}}
{{- end }}

{{/*
Gets the port for the Redis service.

@param {object} . Root template context
@return {string} The Redis port
*/}}
{{- define "clp.redisPort" -}}
{{- if has "redis" .Values.clpConfig.bundled -}}
6379
{{- else -}}
{{- .Values.clpConfig.redis.port -}}
{{- end -}}
{{- end }}

{{/*
Gets the host for the results cache service.

@param {object} . Root template context
@return {string} The results cache host
*/}}
{{- define "clp.resultsCacheHost" -}}
{{- if has "results_cache" .Values.clpConfig.bundled -}}
{{- printf "%s-results-cache" (include "clp.fullname" .) -}}
{{- else -}}
{{- .Values.clpConfig.results_cache.host -}}
{{- end -}}
{{- end }}

{{/*
Gets the port for the results cache service.

@param {object} . Root template context
@return {string} The results cache port
*/}}
{{- define "clp.resultsCachePort" -}}
{{- if has "results_cache" .Values.clpConfig.bundled -}}
27017
{{- else -}}
{{- .Values.clpConfig.results_cache.port -}}
{{- end -}}
{{- end }}

{{/*
Gets the host for the OpenTelemetry Collector service.

@param {object} . Root template context
@return {string} The OpenTelemetry Collector host
*/}}
{{- define "clp.otelCollectorHost" -}}
{{- if has "otel_collector" .Values.clpConfig.bundled -}}
{{- printf "%s-otel-collector" (include "clp.fullname" .) -}}
{{- else -}}
{{- .Values.clpConfig.otel_collector.host -}}
{{- end -}}
{{- end }}

{{/*
Gets the port for the OpenTelemetry Collector service.

@param {object} . Root template context
@return {string} The OpenTelemetry Collector port
*/}}
{{- define "clp.otelCollectorPort" -}}
{{- if has "otel_collector" .Values.clpConfig.bundled -}}
4318
{{- else -}}
{{- .Values.clpConfig.otel_collector.port -}}
{{- end -}}
{{- end }}

{{/*
Gets the URL for the OpenTelemetry Collector service.

@param {object} . Root template context
@return {string} The OpenTelemetry Collector URL
*/}}
{{- define "clp.otelCollectorUrl" -}}
{{- printf
  "http://%s:%s"
  (include "clp.otelCollectorHost" .)
  (include "clp.otelCollectorPort" .) -}}
{{- end }}

{{/*
Gets the host for the Presto service.

@param {object} . Root template context
@return {string} The Presto host
*/}}
{{- define "clp.prestoHost" -}}
{{- if has "presto" .Values.clpConfig.bundled -}}
{{- printf "%s-presto-coordinator" (include "clp.fullname" .) -}}
{{- else -}}
{{- .Values.clpConfig.presto.host -}}
{{- end -}}
{{- end }}

{{/*
Gets the port for the Presto service.

@param {object} . Root template context
@return {string} The Presto port
*/}}
{{- define "clp.prestoPort" -}}
{{- if has "presto" .Values.clpConfig.bundled -}}
8889
{{- else -}}
{{- .Values.clpConfig.presto.port -}}
{{- end -}}
{{- end }}

{{/*
Gets the BROKER_URL env var for Celery workers.

@param {object} . Root template context
@return {string} YAML-formatted env var definition
*/}}
{{- define "clp.celeryBrokerUrlEnvVar" -}}
{{- $user := .Values.credentials.queue.username -}}
{{- $pass := .Values.credentials.queue.password -}}
{{- $host := include "clp.queueHost" . -}}
{{- $port := include "clp.queuePort" . | int -}}
name: "BROKER_URL"
value: {{ printf "amqp://%s:%s@%s:%d" $user $pass $host $port | quote }}
{{- end }}

{{/*
Gets the RESULT_BACKEND env var for Celery workers.

@param {object} root Root template context
@param {string} database Redis database number from config
@return {string} YAML-formatted env var definition
*/}}
{{- define "clp.celeryResultBackendEnvVar" -}}
{{- $pass := .root.Values.credentials.redis.password -}}
{{- $host := include "clp.redisHost" .root -}}
{{- $port := include "clp.redisPort" .root | int -}}
name: "RESULT_BACKEND"
value: {{ printf "redis://default:%s@%s:%d/%d" $pass $host $port (int .database) | quote }}
{{- end }}

{{/*
Creates a volumeMount for the logs input directory.

@return {string} YAML-formatted volumeMount definition
*/}}
{{- define "clp.logsInputVolumeMount" -}}
name: "logs-input"
mountPath: "/mnt/logs"
readOnly: true
{{- end }}

{{/*
Creates a volume for the logs input directory.

@param {object} . Root template context
@return {string} YAML-formatted volume definition
*/}}
{{- define "clp.logsInputVolume" -}}
name: "logs-input"
hostPath:
  path: {{ .Values.clpConfig.logs_input.directory | quote }}
  type: "Directory"
{{- end }}

{{/*
The mount path for the AWS config directory inside containers.

@return {string} Path string
*/}}
{{- define "clp.awsConfigMountPath" -}}/opt/clp/.aws{{- end }}

{{/*
Creates a volumeMount for the AWS config directory.

@param {object} . Root template context
@return {string} YAML-formatted volumeMount definition
*/}}
{{- define "clp.awsConfigVolumeMount" -}}
name: "aws-config"
mountPath: {{ include "clp.awsConfigMountPath" . | quote }}
readOnly: true
{{- end }}

{{/*
Creates a volume for the AWS config directory backed by the chart-managed Secret.

@param {object} . Root template context
@return {string} YAML-formatted volume definition
*/}}
{{- define "clp.awsConfigVolume" -}}
name: "aws-config"
secret:
  secretName: {{ include "clp.fullname" . }}-aws-config
{{- end }}

{{/*
Creates an initContainer that waits for a Kubernetes resource to be ready.

@param {object} root Root template context
@param {string} type The resource type: "service" (waits for pod readiness) or "job" (waits for
completion).
@param {string} name For type="service", this should be the component name. For type="job", this
should be the job name suffix.
@return {string} YAML-formatted initContainer definition
*/}}
{{- define "clp.waitFor" -}}
name: "wait-for-{{ .name }}"
image: {{ include "clp.image.ref" (dict "root" .root "component" "kubectl") | quote }}
imagePullPolicy: {{ .root.Values.image.kubectl.pullPolicy | quote }}
command: [
  "kubectl", "wait",
  {{- if eq .type "service" }}
  "--for=condition=ready",
  "pod", "--selector", "app.kubernetes.io/component={{ .name }}",
  {{- else if eq .type "job" }}
  "--for=condition=complete",
  "job/{{ include "clp.fullname" .root }}-{{ .name }}",
  {{- end }}
  "--timeout=300s"
]
{{- end }}

{{/*
Waits for the results cache to be ready for queries.

When the results cache is bundled, this waits for the results-cache pod to be ready (which requires
both mongod to be accepting connections and the init sidecar to have finished replica set
initialization and index creation). When the results cache is external (3rd-party), this waits for
the results-cache-indices-creator job to complete.

@param {object} root Root template context
@return {string} YAML-formatted initContainer definition
*/}}
{{- define "clp.waitForResultsCache" -}}
{{- if has "results_cache" .Values.clpConfig.bundled -}}
{{- include "clp.waitFor" (dict "root" . "type" "service" "name" "results-cache") -}}
{{- else -}}
{{- include "clp.waitFor" (dict "root" . "type" "job" "name" "results-cache-indices-creator") -}}
{{- end -}}
{{- end }}

{{/*
Creates scheduling configuration (nodeSelector, affinity, tolerations, topologySpreadConstraints)
for a component.

When distributedDeployment is false (single-node mode), a control-plane toleration is automatically
added so pods can be scheduled on tainted control-plane nodes without manual untainting.

@param {object} root Root template context
@param {string} component Key name under .Values.scheduling (e.g., "compressionWorker", "database")
@return {string} YAML-formatted scheduling fields (nodeSelector, affinity, tolerations,
  topologySpreadConstraints)
*/}}
{{- define "clp.createSchedulingConfigs" -}}
{{- $schedulingConfig := index .root.Values.scheduling .component | default dict -}}
{{- $tolerations := $schedulingConfig.tolerations | default list -}}
{{- if not .root.Values.distributedDeployment -}}
{{- $tolerations = append $tolerations (dict
    "key" "node-role.kubernetes.io/control-plane"
    "operator" "Exists"
    "effect" "NoSchedule"
) -}}
{{- end -}}
{{- with $schedulingConfig.nodeSelector }}
nodeSelector:
  {{- toYaml . | nindent 2 }}
{{- end }}
{{- with $schedulingConfig.affinity }}
affinity:
  {{- toYaml . | nindent 2 }}
{{- end }}
{{- with $tolerations }}
tolerations:
  {{- toYaml . | nindent 2 }}
{{- end }}
{{- with $schedulingConfig.topologySpreadConstraints }}
topologySpreadConstraints:
  {{- toYaml . | nindent 2 }}
{{- end }}
{{- end }}{{/* define "clp.createSchedulingConfigs" */}}
