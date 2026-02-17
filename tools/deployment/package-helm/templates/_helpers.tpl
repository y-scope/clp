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
Creates selector labels for matching resources.

@return {string} YAML-formatted selector labels
*/}}
{{- define "clp.selectorLabels" -}}
app.kubernetes.io/name: {{ include "clp.name" . }}
app.kubernetes.io/instance: {{ .Release.Name }}
{{- end }}

{{/*
Creates image reference for the CLP Package.

@return {string} Full image reference (repository:tag)
*/}}
{{- define "clp.image.ref" -}}
{{- $tag := .Values.image.clpPackage.tag | default .Chart.AppVersion }}
{{- printf "%s:%s" .Values.image.clpPackage.repository $tag }}
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
initialDelaySeconds: 60
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
Creates a PersistentVolume that does not use dynamic provisioning.

Behavior depends on the `distributedDeployment` value:
- distributedDeployment=false: Uses local volume type with node affinity targeting control-plane
  nodes
- distributedDeployment=true: Uses hostPath without node affinity (assumes shared storage like NFS)

@param {object} root Root template context
@param {string} component_category (e.g., "database", "shared-data")
@param {string} name (e.g., "archives", "data", "logs")
@param {string} capacity Storage capacity
@param {string[]} accessModes Access modes
@param {string} hostPath Absolute path on host
@return {string} YAML-formatted PersistentVolume resource
*/}}
{{- define "clp.createStaticPv" -}}
apiVersion: "v1"
kind: "PersistentVolume"
metadata:
  name: {{ include "clp.fullname" .root }}-{{ include "clp.volumeName" . }}
  labels:
    {{- include "clp.labels" .root | nindent 4 }}
    app.kubernetes.io/component: {{ .component_category | quote }}
spec:
  capacity:
    storage: {{ .capacity }}
  accessModes: {{ .accessModes }}
  persistentVolumeReclaimPolicy: "Retain"
  storageClassName: {{ .root.Values.storage.storageClassName | quote }}
  {{- if .root.Values.distributedDeployment }}
  hostPath:
    path: {{ .hostPath | quote }}
    type: "DirectoryOrCreate"
  {{- else }}
  local:
    path: {{ .hostPath | quote }}
  nodeAffinity:
    required:
      nodeSelectorTerms:
        - matchExpressions:
            - key: "node-role.kubernetes.io/control-plane"
              operator: "Exists"
  {{- end }}{{/* if .root.Values.distributedDeployment */}}
{{- end }}{{/* define "clp.createStaticPv" */}}

{{/*
Creates a PersistentVolumeClaim for the given component.

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
  storageClassName: {{ .root.Values.storage.storageClassName | quote }}
  selector:
    matchLabels:
      {{- include "clp.selectorLabels" .root | nindent 6 }}
      app.kubernetes.io/component: {{ .component_category | quote }}
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
Checks if a given service is in the bundled list.

@param {object} root Root template context
@param {string} service The service name to check (e.g., "database", "queue", "redis",
  "results_cache")
@return {string} "true" if bundled, empty string otherwise
*/}}
{{- define "clp.isBundled" -}}
{{- if has .service .root.Values.clpConfig.bundled -}}true{{- end -}}
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
value: {{ printf "amqp://%s:%s@%s:%s" $user $pass $host $port | quote }}
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
value: {{ printf "redis://default:%s@%s:%s/%d" $pass $host $port (int .database) | quote }}
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
Creates a volumeMount for the AWS config directory.

@param {object} . Root template context
@return {string} YAML-formatted volumeMount definition
*/}}
{{- define "clp.awsConfigVolumeMount" -}}
name: "aws-config"
mountPath: {{ .Values.clpConfig.aws_config_directory | quote }}
readOnly: true
{{- end }}

{{/*
Creates a volume for the AWS config directory.

@param {object} . Root template context
@return {string} YAML-formatted volume definition
*/}}
{{- define "clp.awsConfigVolume" -}}
name: "aws-config"
hostPath:
  path: {{ .Values.clpConfig.aws_config_directory | quote }}
  type: "Directory"
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
image: "bitnami/kubectl:latest"
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
Creates scheduling configuration (nodeSelector, affinity, tolerations, topologySpreadConstraints)
for a component.

When distributedDeployment is false (single-node mode), a control-plane toleration is automatically
added so pods can be scheduled on tainted control-plane nodes without manual untainting.

@param {object} root Root template context
@param {string} component Key name in top-level Values (e.g., "compressionWorker", "queryWorker")
@return {string} YAML-formatted scheduling fields (nodeSelector, affinity, tolerations,
  topologySpreadConstraints)
*/}}
{{- define "clp.createSchedulingConfigs" -}}
{{- $componentConfig := index .root.Values .component | default dict -}}
{{- $scheduling := $componentConfig.scheduling | default dict -}}
{{- $tolerations := $scheduling.tolerations | default list -}}
{{- if not .root.Values.distributedDeployment -}}
{{- $tolerations = append $tolerations (dict
    "key" "node-role.kubernetes.io/control-plane"
    "operator" "Exists"
    "effect" "NoSchedule"
) -}}
{{- end -}}
{{- with $scheduling.nodeSelector }}
nodeSelector:
  {{- toYaml . | nindent 2 }}
{{- end }}
{{- with $scheduling.affinity }}
affinity:
  {{- toYaml . | nindent 2 }}
{{- end }}
{{- with $tolerations }}
tolerations:
  {{- toYaml . | nindent 2 }}
{{- end }}
{{- with $scheduling.topologySpreadConstraints }}
topologySpreadConstraints:
  {{- toYaml . | nindent 2 }}
{{- end }}
{{- end }}{{/* define "clp.createSchedulingConfigs" */}}
