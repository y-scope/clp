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
Creates a local PersistentVolume.

@param {object} root Root template context
@param {string} name PV name
@param {string} component Component label
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
  name: {{ .name }}
  labels:
    {{- include "clp.labels" .root | nindent 4 }}
    app.kubernetes.io/component: {{ .component | quote }}
spec:
  capacity:
    storage: {{ .capacity }}
  accessModes: {{ .accessModes }}
  persistentVolumeReclaimPolicy: "Retain"
  storageClassName: "local-storage"
  local:
    path: {{ .hostPath }}
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
