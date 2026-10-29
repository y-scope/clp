# syntax=docker/dockerfile:1
#
# Extends the CLP musllinux_1_2 build image with tools needed for .apk
# packaging: patchelf (to rewrite RPATH) and alpine-sdk (for abuild).
#
# Usage:
#   docker build --build-arg BASE_IMAGE=clp-core-dependencies-x86-musllinux_1_2:dev ...

ARG BASE_IMAGE
FROM ${BASE_IMAGE}

RUN apk add --no-cache patchelf alpine-sdk
