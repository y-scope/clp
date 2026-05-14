# CLP Telemetry Server

This directory contains the Docker Compose stack for the CLP server-side telemetry infrastructure.

## Stack Components
- OpenTelemetry Collector: Receives OTLP/HTTP from clients and writes to ClickHouse.
- ClickHouse: Stores metrics data.
- Grafana: Visualizes metrics with a pre-configured ClickHouse datasource and basic dashboard.
- Caddy: Reverse proxy with automatic Let's Encrypt TLS.

## Setup
1. Ensure your DNS (`telemetry.yscope.io`) points to the machine running this stack.
2. Run `docker compose up -d`
