FROM gentoo/stage3:latest

ARG VERSION=1.0.0
WORKDIR /build

ENV FEATURES="-ipc -network-sandbox"

RUN emerge-webrsync
RUN mkdir -p /etc/portage/repos.conf
RUN printf "[clockout]\nlocation = /var/db/repos/clockout\nmasters = gentoo\npriority = 100\n" > /etc/portage/repos.conf/clockout.conf
RUN mkdir -p /var/db/repos/clockout/app-misc/clockout

COPY packaging/gentoo/clockout.ebuild.in /tmp/clockout.ebuild.in
COPY . /build/src
RUN mkdir -p /tmp/sources/clockout-${VERSION}
RUN cp -r /build/src/* /tmp/sources/clockout-${VERSION}/
RUN tar -czf /var/cache/distfiles/clockout-${VERSION}.tar.gz -C /tmp/sources clockout-${VERSION}
RUN sed -e "s/@VERSION@/${VERSION}/g" /tmp/clockout.ebuild.in > /var/db/repos/clockout/app-misc/clockout/clockout-${VERSION}.ebuild
RUN chown -R portage:portage /var/db/repos/clockout /var/cache/distfiles

RUN emerge --quiet-build=y --oneshot sys-libs/ncurses dev-libs/json-c
RUN ebuild /var/db/repos/clockout/app-misc/clockout/clockout-${VERSION}.ebuild manifest
RUN emerge --quiet-build=y -B app-misc/clockout
RUN mkdir -p /dist && find /var/cache/binpkgs -name 'clockout-*.tbz2' -exec cp {} /dist/ \;

CMD ["/bin/bash"]
