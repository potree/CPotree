FROM ubuntu:23.10

RUN --mount=type=cache,target=/var/lib/apt/lists \
  --mount=type=cache,target=/var/cache,sharing=locked \
  apt-get update \
  && apt-get install --yes build-essential git cmake python3 \
  zlib1g-dev libssl-dev libcurlpp-dev

WORKDIR /app

COPY . .

WORKDIR /app/build

RUN --mount=type=cache,target=/app/build \
  cmake .. \
  && make \
  && cp extract_area extract_profile /usr/bin \
  && cp liblaszip.so /usr/lib

RUN extract_profile --help
