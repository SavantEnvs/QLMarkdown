# syntax=docker/dockerfile:1.7
#
# Commit image for the QLMarkdown fuzz targets, on the org C/C++ base.
#   DOCKER_BUILDKIT=1 docker build -f mayhem/Dockerfile --build-arg COMMIT=$(git rev-parse HEAD) -t qlmarkdown-commit .
FROM ghcr.io/savantenvs/base:20261023

ARG COMMIT
ARG REPO_URL
ARG SOURCE_DATE_EPOCH
ARG SANITIZER_FLAGS
ARG COVERAGE_FLAGS=
ARG MAYHEM_JOBS=

# libpcre2-dev: the `heads` extension links PCRE2 (32-bit code units). locales: that extension case-maps
# titles under en_US.UTF-8 and prints "setlocale failed" on every heading without it.
USER root
RUN [ -n "$SOURCE_DATE_EPOCH" ] && repro-sources-list.sh || true ; \
    apt-get update && apt-get install -yq --no-install-recommends libpcre2-dev locales \
 && sed -i 's/^# *en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen && locale-gen \
 && rm -rf /var/lib/apt/lists/*

# Dependencies that upstream tracks as git submodules (empty in the checkout), fetched by commit and
# kept as source under /opt/toolchains/deps (SPEC 6.2 item 19). build.sh builds cmark-gfm out of tree.
RUN fetch() { d=/opt/toolchains/deps/$1/src; mkdir -p "$d" && cd "$d" && git init -q . \
      && git fetch -q --depth 1 "$2" "$3" && git checkout -q FETCH_HEAD && rm -rf .git; } \
 && fetch cmark-gfm-27d942c https://github.com/github/cmark-gfm 27d942c8b0a62d192f616e5bf3578f4b6a89e180 \
 && fetch jpcre2-867af2d https://github.com/jpcre2/jpcre2 867af2d36413cf5f69c2e224f4f96f56f876d889 \
 && chmod -R a+rX /opt/toolchains/deps
USER mayhem

WORKDIR /mayhem
COPY --chown=2000:2000 . /mayhem

RUN if [ -x /opt/mayhem/ccache/prefill ]; then /opt/mayhem/ccache/prefill bash mayhem/build.sh; \
    else bash mayhem/build.sh; fi

RUN if [ -f mayhem/test.sh ]; then bash mayhem/test.sh; else echo "no mayhem/test.sh — repo ships no functional tests"; fi

LABEL org.opencontainers.image.source="${REPO_URL}" \
      org.opencontainers.image.revision="${COMMIT}" \
      rlenv.tier="commit" \
      rlenv.source_date_epoch="${SOURCE_DATE_EPOCH}" \
      rlenv.user="mayhem"

USER mayhem
WORKDIR /mayhem
