#!/usr/bin/env bash

# Emits package-manager CA options when the ca-trust library staged a host CA
# bundle for this build step. Sourced by the scripts that invoke a package
# manager over the network.
#
# apt and dnf do NOT read SSL_CERT_FILE or CURL_CA_BUNDLE -- verified: apt over
# https still fails certificate verification with both set to a valid bundle,
# and dnf succeeds with both set to /dev/null -- so they have to be pointed at
# the bundle explicitly. curl, pip, and apk do honour the environment variables
# that container.sh exports and need nothing from this file.
#
# A no-op when CA_TRUST_DIR is unset or the staged bundle is empty, which is the
# default and CI case.
#
# Sets:
#   CLP_APT_CA_OPTS  Options for apt-get (bash array; may be empty)
#   CLP_DNF_CA_OPTS  Options for dnf (bash array; may be empty)

CLP_APT_CA_OPTS=()
CLP_DNF_CA_OPTS=()

# The staged filename is the ca-trust library's CA_TRUST_BUNDLE_FILENAME. It's
# spelled out here because that constant lives in the library's host-side half,
# which isn't available inside the container.
_clp_ca_trust_bundle="${CA_TRUST_DIR:-}/ca-bundle.pem"

if [[ -n "${CA_TRUST_DIR:-}" && -s "${_clp_ca_trust_bundle}" ]]; then
    CLP_APT_CA_OPTS=(-o "Acquire::https::CaInfo=${_clp_ca_trust_bundle}")
    CLP_DNF_CA_OPTS=("--setopt=sslcacert=${_clp_ca_trust_bundle}")
fi

unset _clp_ca_trust_bundle
