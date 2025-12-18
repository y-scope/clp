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
Creates a local PersistentVolume.

@param {object} root Root template context
@param {string} component_category (e.g., "database", "shared-data")
@param {string} name (e.g., "archives", "data", "logs")
@param {string} nodeRole Node role for affinity. Targets nodes with label
  "node-role.kubernetes.io/<nodeRole>". Always falls back to
  "node-role.kubernetes.io/control-plane"
@param {string} capacity Storage capacity
@param {string[]} accessModes Access modes
@param {string} hostPath Absolute path on host
@return {string} YAML-formatted PersistentVolume resource
*/}}
{{- define "clp.createLocalPv" -}}
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
  storageClassName: "local-storage"
  local:
    path: {{ .hostPath | quote }}
  nodeAffinity:
    required:
      nodeSelectorTerms:
        - matchExpressions:
            - key: {{ printf "node-role.kubernetes.io/%s" .nodeRole | quote }}
              operator: "Exists"
        - matchExpressions:
            - key: "node-role.kubernetes.io/control-plane"
              operator: "Exists"
{{- end }}

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
  storageClassName: "local-storage"
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
Gets the BROKER_URL env var for Celery workers.

@param {object} . Root template context
@return {string} YAML-formatted env var definition
*/}}
{{- define "clp.celeryBrokerUrlEnvVar" -}}
{{- $user := .Values.credentials.queue.username -}}
{{- $pass := .Values.credentials.queue.password -}}
{{- $host := printf "%s-queue" (include "clp.fullname" .) -}}
name: "BROKER_URL"
value: {{ printf "amqp://%s:%s@%s:5672" $user $pass $host | quote }}
{{- end }}

{{/*
Gets the RESULT_BACKEND env var for Celery workers.

@param {object} root Root template context
@param {string} database Redis database number from config
@return {string} YAML-formatted env var definition
*/}}
{{- define "clp.celeryResultBackendEnvVar" -}}
{{- $pass := .root.Values.credentials.redis.password -}}
{{- $host := printf "%s-redis" (include "clp.fullname" .root) -}}
name: "RESULT_BACKEND"
value: {{ printf "redis://default:%s@%s:6379/%d" $pass $host (int .database) | quote }}
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
