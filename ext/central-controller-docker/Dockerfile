# Central Controller -- conda-free, CMake-based, Debian trixie. Multi-stage:
#   builder  : apt deps + scripts/bootstrap-deps.sh (OTel 1.27 + google-cloud-cpp
#              2.38 from source) + CMake build of zerotier-one
#   go_base  : golang-migrate
#   runtime  : slim trixie with exactly the runtime libs the binary needs
#
# Build (context = repo root, see .dockerignore):
#   docker build -f ext/central-controller-docker/Dockerfile -t ztctl .
FROM debian:trixie AS builder

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build git pkg-config ca-certificates curl \
        libpqxx-dev libpq-dev libhiredis-dev libjemalloc-dev nlohmann-json3-dev \
        libabsl-dev libprotobuf-dev protobuf-compiler protobuf-compiler-grpc libgrpc++-dev \
        libgtest-dev libgmock-dev libssl-dev libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs \
        | sh -s -- -y --default-toolchain 1.89.0 --profile minimal
ENV PATH="/root/.cargo/bin:${PATH}"

WORKDIR /ZeroTierOne

# Pinned from-source deps -> /opt/zt-deps, against apt's abseil/protobuf/grpc.
# ZT_CONTROLLER_DEPS=1 builds the full controller set (OTel SDK + OTLP exporters +
# google-cloud-cpp); without it the script only builds the header-only OTel API.
# Copy only the bootstrap script first so this expensive layer stays cached across
# changes to the rest of the tree.
COPY scripts/bootstrap-deps.sh scripts/bootstrap-deps.sh
RUN ZT_CONTROLLER_DEPS=1 bash scripts/bootstrap-deps.sh /opt/zt-deps

COPY . .
RUN cmake -DCMAKE_BUILD_TYPE=Release -DZT1_CENTRAL_CONTROLLER=1 \
        -DCMAKE_PREFIX_PATH=/opt/zt-deps -S . -B build-cmake -G Ninja \
    && cmake --build build-cmake -j"$(nproc)"

# Compute the exact runtime apt packages the binary links (robust against the
# trixie t64 package renames). The two non-apt OTel proto .so's are copied
# explicitly in the runtime stage, so dpkg-S misses there are expected.
# readlink -f resolves the usrmerge /lib -> /usr/lib symlinks so dpkg -S can
# attribute each .so to its package (and naturally picks up the trixie t64 names).
RUN ldd build-cmake/zerotier-one | awk '/=> \//{print $3}' | xargs -r readlink -f \
        | xargs -r dpkg -S 2>/dev/null | cut -d: -f1 | tr ',' '\n' | sort -u > /opt/runtime-pkgs.txt \
    && echo "--- runtime packages ---" && cat /opt/runtime-pkgs.txt

# ---------------------------------------------------------------------------
FROM golang:1.24-bookworm AS go_base
RUN GOFLAGS='-p=1' go install -tags 'postgres' github.com/golang-migrate/migrate/v4/cmd/migrate@latest

# ---------------------------------------------------------------------------
FROM debian:trixie AS runtime
COPY --from=builder /opt/runtime-pkgs.txt /opt/runtime-pkgs.txt
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        $(cat /opt/runtime-pkgs.txt) \
        libjemalloc2 postgresql-client netcat-openbsd ca-certificates \
    && rm -rf /var/lib/apt/lists/* /opt/runtime-pkgs.txt

COPY --from=builder /ZeroTierOne/build-cmake/zerotier-one /usr/local/bin/zerotier-one
# The only dynamically-linked from-source libs (OTel/gcloud are otherwise static).
COPY --from=builder /opt/zt-deps/lib/libopentelemetry_proto.so /opt/zt-deps/lib/libopentelemetry_proto_grpc.so /usr/local/lib/
RUN echo /usr/local/lib > /etc/ld.so.conf.d/zerotier.conf && ldconfig

COPY --from=go_base /go/bin/migrate /usr/local/bin/migrate
COPY ext/central-controller-docker/migrations /migrations
COPY ext/central-controller-docker/main.sh /main.sh
RUN chmod +x /usr/local/bin/zerotier-one /usr/local/bin/migrate /main.sh

ENTRYPOINT ["/main.sh"]
