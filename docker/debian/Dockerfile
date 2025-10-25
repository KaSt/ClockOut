FROM debian:stable-slim

ARG VERSION=1.0.0
WORKDIR /src

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
       build-essential \
       libncursesw6 \
       libncurses-dev \
       libjson-c5 \
       libjson-c-dev \
       ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY . /src
RUN make

RUN mkdir -p /src/pkg/usr/local/bin /src/pkg/DEBIAN /dist
RUN install -Dm755 clockout /src/pkg/usr/local/bin/clockout
RUN install -Dm644 README.md /src/pkg/usr/share/doc/clockout/README.md
RUN cat <<CONTROL > /src/pkg/DEBIAN/control
Package: clockout
Version: ${VERSION}
Section: utils
Priority: optional
Architecture: amd64
Depends: libjson-c5, libncursesw6
Maintainer: ClockOut Maintainers <maintainers@example.com>
Description: Terminal companion for tracking the rest of your workday
 ClockOut renders a large countdown timer or discrete blocks to help you
 understand how much of your scheduled shift remains.
CONTROL

RUN dpkg-deb --build /src/pkg /dist/clockout_${VERSION}_amd64.deb

CMD ["/bin/bash"]
