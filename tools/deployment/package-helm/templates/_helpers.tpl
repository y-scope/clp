{{/*
Expand the name of the chart.
*/}}
{{- define "clp.name" -}}
{{- default .Chart.Name .Values.nameOverride | trunc 63 | trimSuffix "-" }}
{{- end }}

{{/*
Create a default fully qualified app name.
We truncate at 63 chars because some Kubernetes name fields are limited to this (by the DNS naming spec).
If release name contains chart name it will be used as a full name.
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
Create chart name and version as used by the chart label.
*/}}
{{- define "clp.chart" -}}
{{- printf "%s-%s" .Chart.Name .Chart.Version | replace "+" "_" | trunc 63 | trimSuffix "-" }}
{{- end }}

{{/*
Common labels
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
Selector labels
*/}}
{{- define "clp.selectorLabels" -}}
app.kubernetes.io/name: {{ include "clp.name" . }}
app.kubernetes.io/instance: {{ .Release.Name }}
{{- end }}

{{/*
Image reference for CLP Package
*/}}
{{- define "clp.image.ref" -}}
{{- $tag := .Values.image.clpPackage.tag | default .Chart.AppVersion }}
{{- printf "%s:%s" .Values.image.clpPackage.repository $tag }}
{{- end }}

{{/*
Creates timings for readiness probes (faster checks for quicker startup).
*/}}
{{- define "clp.readinessProbeTimings" -}}
initialDelaySeconds: 1
periodSeconds: 1
timeoutSeconds: 5
failureThreshold: 10
{{- end }}

{{/*
Creates timings for liveness probes.
*/}}
{{- define "clp.livenessProbeTimings" -}}
initialDelaySeconds: 30
periodSeconds: 10
timeoutSeconds: 5
failureThreshold: 3
{{- end }}

{{/*
Creates a local PersistentVolume.
Parameters (dict):
  root: Root template context
  name: PV name (full)
  component: Component label
  capacity: Storage capacity
  accessModes: Access modes (list)
  hostPath: Absolute path on host
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
            - key: "kubernetes.io/hostname"
              operator: "Exists"
{{- end }}

{{/*
Creates a PersistentVolumeClaim.
Parameters (dict):
  root: Root template context
  component: Component label (used in name, labels, and selectors)
  capacity: Storage capacity
  accessModes: Access modes (list)
*/}}
{{- define "clp.createPvc" -}}
apiVersion: "v1"
kind: "PersistentVolumeClaim"
metadata:
  name: {{ include "clp.fullname" .root }}-{{ .component }}
  labels:
    {{- include "clp.labels" .root | nindent 4 }}
    app.kubernetes.io/component: {{ .component | quote }}
spec:
  accessModes: {{ .accessModes }}
  storageClassName: "local-storage"
  selector:
    matchLabels:
      {{- include "clp.selectorLabels" .root | nindent 6 }}
      app.kubernetes.io/component: {{ .component | quote }}
  resources:
    requests:
      storage: {{ .capacity }}
{{- end }}
