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
timeoutSeconds: 1
failureThreshold: 3
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
CLP logs directory path on host
*/}}
{{- define "clp.logsDirHost" -}}
{{ .Values.storage.localPathBase }}/{{ .Values.clpConfig.logs_directory }}
{{- end }}

{{/*
CLP data directory path on host
*/}}
{{- define "clp.dataDirHost" -}}
{{ .Values.storage.localPathBase }}/{{ .Values.clpConfig.data_directory }}
{{- end }}

{{/*
Creates a local PersistentVolume for data storage.
*/}}
{{- define "clp.createLocalDataPv" -}}
{{- $root := index . 0 -}}
{{- $component := index . 1 -}}
apiVersion: "v1"
kind: "PersistentVolume"
metadata:
  name: {{ include "clp.fullname" $root }}-data-pv-{{ $component }}
  labels:
    {{- include "clp.labels" $root | nindent 4 }}
    app.kubernetes.io/component: {{ $component | quote }}
spec:
  capacity:
    storage: "20Gi"
  accessModes: ["ReadWriteOnce"]
  persistentVolumeReclaimPolicy: "Retain"
  storageClassName: "local-storage"
  local:
    path: {{ include "clp.dataDirHost" $root }}/{{ $component }}
  nodeAffinity:
    required:
      nodeSelectorTerms:
        - matchExpressions:
            - key: "kubernetes.io/hostname"
              operator: "Exists"
{{- end }}

{{/*
Creates a local PersistentVolume for logs storage.
*/}}
{{- define "clp.createLocalLogsPv" -}}
{{- $root := index . 0 -}}
{{- $component := index . 1 -}}
apiVersion: "v1"
kind: "PersistentVolume"
metadata:
  name: {{ include "clp.fullname" $root }}-logs-pv-{{ $component }}
  labels:
    {{- include "clp.labels" $root | nindent 4 }}
    app.kubernetes.io/component: {{ $component | quote }}
spec:
  capacity:
    storage: "5Gi"
  accessModes: ["ReadWriteOnce"]
  persistentVolumeReclaimPolicy: "Retain"
  storageClassName: "local-storage"
  local:
    path: {{ include "clp.logsDirHost" $root }}/{{ $component }}
  nodeAffinity:
    required:
      nodeSelectorTerms:
        - matchExpressions:
            - key: "kubernetes.io/hostname"
              operator: "Exists"
{{- end }}

{{/*
Creates a local PersistentVolume for streams storage (ReadWriteMany).
*/}}
{{- define "clp.createStreamsPv" -}}
{{- $root := . -}}
apiVersion: "v1"
kind: "PersistentVolume"
metadata:
  name: {{ include "clp.fullname" $root }}-streams-pv
  labels:
    {{- include "clp.labels" $root | nindent 4 }}
    app.kubernetes.io/component: "streams"
spec:
  capacity:
    storage: "20Gi"
  accessModes: ["ReadWriteMany"]
  persistentVolumeReclaimPolicy: "Retain"
  storageClassName: "local-storage"
  local:
    path: {{ include "clp.dataDirHost" $root }}/streams
  nodeAffinity:
    required:
      nodeSelectorTerms:
        - matchExpressions:
            - key: "kubernetes.io/hostname"
              operator: "Exists"
{{- end }}

{{/*
Creates a local PersistentVolume for tmp storage (ReadWriteMany).
*/}}
{{- define "clp.createTmpPv" -}}
{{- $root := . -}}
apiVersion: "v1"
kind: "PersistentVolume"
metadata:
  name: {{ include "clp.fullname" $root }}-tmp-pv
  labels:
    {{- include "clp.labels" $root | nindent 4 }}
    app.kubernetes.io/component: "tmp"
spec:
  capacity:
    storage: "10Gi"
  accessModes: ["ReadWriteMany"]
  persistentVolumeReclaimPolicy: "Retain"
  storageClassName: "local-storage"
  local:
    path: {{ .Values.storage.localPathBase }}/var/tmp
  nodeAffinity:
    required:
      nodeSelectorTerms:
        - matchExpressions:
            - key: "kubernetes.io/hostname"
              operator: "Exists"
{{- end }}

{{/*
Creates a local PersistentVolume for archives storage (ReadWriteMany).
*/}}
{{- define "clp.createArchivesPv" -}}
{{- $root := . -}}
apiVersion: "v1"
kind: "PersistentVolume"
metadata:
  name: {{ include "clp.fullname" $root }}-archives-pv
  labels:
    {{- include "clp.labels" $root | nindent 4 }}
    app.kubernetes.io/component: "archives"
spec:
  capacity:
    storage: "50Gi"
  accessModes: ["ReadWriteMany"]
  persistentVolumeReclaimPolicy: "Retain"
  storageClassName: "local-storage"
  local:
    path: {{ include "clp.dataDirHost" $root }}/archives
  nodeAffinity:
    required:
      nodeSelectorTerms:
        - matchExpressions:
            - key: "kubernetes.io/hostname"
              operator: "Exists"
{{- end }}

{{/*
Creates a local PersistentVolume for staged-archives storage (ReadWriteMany).
*/}}
{{- define "clp.createStagedArchivesPv" -}}
{{- $root := . -}}
apiVersion: "v1"
kind: "PersistentVolume"
metadata:
  name: {{ include "clp.fullname" $root }}-staged-archives-pv
  labels:
    {{- include "clp.labels" $root | nindent 4 }}
    app.kubernetes.io/component: "staged-archives"
spec:
  capacity:
    storage: "20Gi"
  accessModes: ["ReadWriteMany"]
  persistentVolumeReclaimPolicy: "Retain"
  storageClassName: "local-storage"
  local:
    path: {{ include "clp.dataDirHost" $root }}/staged-archives
  nodeAffinity:
    required:
      nodeSelectorTerms:
        - matchExpressions:
            - key: "kubernetes.io/hostname"
              operator: "Exists"
{{- end }}

{{/*
Creates a local PersistentVolume for staged-streams storage (ReadWriteMany).
*/}}
{{- define "clp.createStagedStreamsPv" -}}
{{- $root := . -}}
apiVersion: "v1"
kind: "PersistentVolume"
metadata:
  name: {{ include "clp.fullname" $root }}-staged-streams-pv
  labels:
    {{- include "clp.labels" $root | nindent 4 }}
    app.kubernetes.io/component: "staged-streams"
spec:
  capacity:
    storage: "20Gi"
  accessModes: ["ReadWriteMany"]
  persistentVolumeReclaimPolicy: "Retain"
  storageClassName: "local-storage"
  local:
    path: {{ include "clp.dataDirHost" $root }}/staged-streams
  nodeAffinity:
    required:
      nodeSelectorTerms:
        - matchExpressions:
            - key: "kubernetes.io/hostname"
              operator: "Exists"
{{- end }}

