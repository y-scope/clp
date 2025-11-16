{{/*
Expand the name of the chart.
*/}}
{{- define "clp-package.name" -}}
{{- default .Chart.Name .Values.nameOverride | trunc 63 | trimSuffix "-" }}
{{- end }}

{{/*
Create a default fully qualified app name.
We truncate at 63 chars because some Kubernetes name fields are limited to this (by the DNS naming spec).
If release name contains chart name it will be used as a full name.
*/}}
{{- define "clp-package.fullname" -}}
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
{{- define "clp-package.chart" -}}
{{- printf "%s-%s" .Chart.Name .Chart.Version | replace "+" "_" | trunc 63 | trimSuffix "-" }}
{{- end }}

{{/*
Common labels
*/}}
{{- define "clp-package.labels" -}}
helm.sh/chart: {{ include "clp-package.chart" . }}
{{ include "clp-package.selectorLabels" . }}
{{- if .Chart.AppVersion }}
app.kubernetes.io/version: {{ .Chart.AppVersion | quote }}
{{- end }}
app.kubernetes.io/managed-by: {{ .Release.Service }}
{{- end }}

{{/*
Selector labels
*/}}
{{- define "clp-package.selectorLabels" -}}
app.kubernetes.io/name: {{ include "clp-package.name" . }}
app.kubernetes.io/instance: {{ .Release.Name }}
{{- end }}

{{/*
Image reference for CLP Package
*/}}
{{- define "clp-package.image.ref" -}}
{{- $tag := .Values.image.clpPackage.tag | default .Chart.AppVersion }}
{{- printf "%s:%s" .Values.image.clpPackage.repository $tag }}
{{- end }}

{{/*
Require a value with a standard error message
*/}}
{{- define "clp-package.requireInput" -}}
{{- . | required "Please set a value." }}
{{- end }}

{{/*
Creates timings for readiness probes (faster checks for quicker startup).
*/}}
{{- define "clp-package.readinessProbeTimings" -}}
initialDelaySeconds: 1
periodSeconds: 1
timeoutSeconds: 2
failureThreshold: 3
{{- end }}

{{/*
Creates timings for liveness probes.
*/}}
{{- define "clp-package.livenessProbeTimings" -}}
initialDelaySeconds: 30
periodSeconds: 10
timeoutSeconds: 5
failureThreshold: 3
{{- end }}

{{/*
CLP logs directory path on host
*/}}
{{- define "clp-package.logsDirHost" -}}
{{ .Values.storage.localPathBase }}/{{ .Values.clpConfig.logs_directory }}
{{- end }}

{{/*
CLP data directory path on host
*/}}
{{- define "clp-package.dataDirHost" -}}
{{ .Values.storage.localPathBase }}/{{ .Values.clpConfig.data_directory }}
{{- end }}

{{/*
Creates a local PersistentVolume for data storage.
*/}}
{{- define "clp-package.createLocalDataPv" -}}
{{- $root := index . 0 -}}
{{- $component := index . 1 -}}
apiVersion: "v1"
kind: "PersistentVolume"
metadata:
  name: {{ include "clp-package.fullname" $root }}-data-pv-{{ $component }}
  labels:
    {{- include "clp-package.labels" $root | nindent 4 }}
    app.kubernetes.io/component: {{ $component | quote }}
spec:
  capacity:
    storage: "20Gi"
  accessModes: ["ReadWriteOnce"]
  persistentVolumeReclaimPolicy: "Retain"
  storageClassName: "local-storage"
  local:
    path: {{ include "clp-package.dataDirHost" $root }}/{{ $component }}
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
{{- define "clp-package.createLocalLogsPv" -}}
{{- $root := index . 0 -}}
{{- $component := index . 1 -}}
apiVersion: "v1"
kind: "PersistentVolume"
metadata:
  name: {{ include "clp-package.fullname" $root }}-logs-pv-{{ $component }}
  labels:
    {{- include "clp-package.labels" $root | nindent 4 }}
    app.kubernetes.io/component: {{ $component | quote }}
spec:
  capacity:
    storage: "5Gi"
  accessModes: ["ReadWriteOnce"]
  persistentVolumeReclaimPolicy: "Retain"
  storageClassName: "local-storage"
  local:
    path: {{ include "clp-package.logsDirHost" $root }}/{{ $component }}
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
{{- define "clp-package.createStreamsPv" -}}
{{- $root := . -}}
apiVersion: "v1"
kind: "PersistentVolume"
metadata:
  name: {{ include "clp-package.fullname" $root }}-streams-pv
  labels:
    {{- include "clp-package.labels" $root | nindent 4 }}
    app.kubernetes.io/component: "streams"
spec:
  capacity:
    storage: "20Gi"
  accessModes: ["ReadWriteMany"]
  persistentVolumeReclaimPolicy: "Retain"
  storageClassName: "local-storage"
  local:
    path: {{ include "clp-package.dataDirHost" $root }}/streams
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
{{- define "clp-package.createTmpPv" -}}
{{- $root := . -}}
apiVersion: "v1"
kind: "PersistentVolume"
metadata:
  name: {{ include "clp-package.fullname" $root }}-tmp-pv
  labels:
    {{- include "clp-package.labels" $root | nindent 4 }}
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
{{- define "clp-package.createArchivesPv" -}}
{{- $root := . -}}
apiVersion: "v1"
kind: "PersistentVolume"
metadata:
  name: {{ include "clp-package.fullname" $root }}-archives-pv
  labels:
    {{- include "clp-package.labels" $root | nindent 4 }}
    app.kubernetes.io/component: "archives"
spec:
  capacity:
    storage: "50Gi"
  accessModes: ["ReadWriteMany"]
  persistentVolumeReclaimPolicy: "Retain"
  storageClassName: "local-storage"
  local:
    path: {{ include "clp-package.dataDirHost" $root }}/archives
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
{{- define "clp-package.createStagedArchivesPv" -}}
{{- $root := . -}}
apiVersion: "v1"
kind: "PersistentVolume"
metadata:
  name: {{ include "clp-package.fullname" $root }}-staged-archives-pv
  labels:
    {{- include "clp-package.labels" $root | nindent 4 }}
    app.kubernetes.io/component: "staged-archives"
spec:
  capacity:
    storage: "20Gi"
  accessModes: ["ReadWriteMany"]
  persistentVolumeReclaimPolicy: "Retain"
  storageClassName: "local-storage"
  local:
    path: {{ include "clp-package.dataDirHost" $root }}/staged-archives
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
{{- define "clp-package.createStagedStreamsPv" -}}
{{- $root := . -}}
apiVersion: "v1"
kind: "PersistentVolume"
metadata:
  name: {{ include "clp-package.fullname" $root }}-staged-streams-pv
  labels:
    {{- include "clp-package.labels" $root | nindent 4 }}
    app.kubernetes.io/component: "staged-streams"
spec:
  capacity:
    storage: "20Gi"
  accessModes: ["ReadWriteMany"]
  persistentVolumeReclaimPolicy: "Retain"
  storageClassName: "local-storage"
  local:
    path: {{ include "clp-package.dataDirHost" $root }}/staged-streams
  nodeAffinity:
    required:
      nodeSelectorTerms:
        - matchExpressions:
            - key: "kubernetes.io/hostname"
              operator: "Exists"
{{- end }}

