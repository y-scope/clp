# CLP Single-Container Image Compliance Notes

The single-container image is intended to package CLP with its local runtime
dependencies for development, demos, and constrained deployments. Unlike the
Docker Compose deployment, publishing this image redistributes third-party
service binaries in the CLP image.

Before publishing an official image:

1. Generate and publish an SBOM for the exact image digest.
2. Publish the generated third-party notices from
   `/opt/clp/share/third-party-notices`.
3. Publish corresponding source artifacts for copyleft and source-available
   components in the image.
4. Keep MongoDB embedded-only. It is bundled solely as CLP's internal
   results-cache implementation detail and must not be exposed or marketed as a
   general-purpose MongoDB service.
5. Keep Redis below version 7.4 unless the license review is updated.
6. Do not describe the image as Apache-2.0 only. CLP is Apache-2.0, but the
   image includes third-party components under their own licenses.

The default launcher publishes only CLP-facing ports. MariaDB, RabbitMQ, Redis,
and MongoDB bind to loopback inside the container by default. Publishing those
internal service ports requires an explicit debug opt-in.
