FROM ubuntu:22.04

ARG GITHUB_SHA
LABEL org.opencontainers.image.revision="${GITHUB_SHA}"

ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /app

RUN apt-get update && apt-get install -y \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/*

COPY ./build/lobby_server /app/lobby_server
COPY ./scripts /app/scripts
COPY ./config.ini /app/config.ini
COPY ./games.ini /app/games.ini

RUN chmod +x /app/lobby_server
RUN mkdir /app/logs

RUN useradd -m -s /bin/bash lobbyuser
USER lobbyuser

EXPOSE 8080
CMD ["./lobby_server"]
