FROM rust:1.85 AS builder
WORKDIR /app
COPY Cargo.toml Cargo.lock* ./
COPY src ./src
RUN cargo build --release

FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates && rm -rf /var/lib/apt/lists/*
COPY --from=builder /app/target/release/clp-telemetry-server /usr/local/bin/
EXPOSE 8080
CMD ["clp-telemetry-server"]
