# Building the single-container image

The single-container image is layered on top of the regular `clp-package` image. Build the release
artifact flow with:

```shell
task docker-images:package-single
```

This task first builds the package and base package image, then runs
`tools/deployment/package/build-single-image.sh`. The resulting image is tagged as
`clp-package-single:latest`, and its image ID is written to `build/clp-package-single-image.id`.
It then runs `tools/deployment/package/generate-single-image-compliance-bundle.sh` to write the
release compliance bundle under `build/clp-package-single-compliance`.

Because this is the release artifact flow, `syft` must be available on `PATH` so the compliance
bundle can include SBOM files.

## Base image reference

The single-container image build creates a temporary local base-image tag,
`clp-package:single-base`, because Docker `FROM` needs an image reference rather than the raw image
ID stored in `build/clp-package-image.id`. The tag is removed when the build exits.

## Runtime flow

No repository-side runner is required to start the image. Users run the image directly with
`docker run`, mounting `clp-config.yaml`, `credentials.yaml`, persistent data/log/tmp directories,
and any filesystem log-input directories they need. Inside the container, the entrypoint:

* loads the CLP config and credentials;
* writes a container-runtime config to `/run/clp-single-container/clp-config.yaml`;
* exports the service credentials and connection values expected by the existing CLP Python
  initialization scripts;
* starts Supervisor, which launches the backing services, DB/results-cache initialization jobs,
  schedulers, workers, API server, log-ingestor, optional MCP server, and WebUI.

The DB and results-cache initialization steps intentionally use the existing Python modules:

* `clp_py_utils.create-db-tables`
* `clp_py_utils.initialize-results-cache`

Redis does not have a separate initialization step, matching the Docker Compose flow.

## Compliance Bundle

The image build itself runs the release-license guard and generates third-party notice files inside
the image. The compliance bundle is generated as part of `task docker-images:package-single`; it
collects the notice files from the image and generates SBOM files for release review.
