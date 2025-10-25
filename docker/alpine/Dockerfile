FROM alpine:3.18

ARG VERSION=1.0.0
ENV PACKAGER="ClockOut Maintainers <maintainers@example.com>"

RUN apk add --no-cache build-base abuild sudo ncurses-dev json-c-dev ncurses-libs json-c

RUN adduser -D builder \
    && addgroup builder abuild \
    && echo 'builder ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

WORKDIR /home/builder
COPY . /home/builder/src
RUN chown -R builder:abuild /home/builder

USER builder
RUN mkdir -p packages \
    && mkdir -p src-dist/clockout-${VERSION} \
    && mkdir -p packaging
RUN cp -r src/* src-dist/clockout-${VERSION}/
RUN tar -czf packaging/clockout-${VERSION}.tar.gz -C src-dist clockout-${VERSION}
RUN cp src/packaging/alpine/APKBUILD packaging/APKBUILD
WORKDIR /home/builder/packaging
RUN abuild-keygen -a -n
RUN KEY=$(ls /home/builder/.abuild/*.rsa | head -n1) && printf 'PACKAGER="%s"\nPACKAGER_PRIVKEY="%s"\n' "${PACKAGER}" "$KEY" >> /home/builder/.abuild/abuild.conf
RUN mkdir -p /dist
RUN abuild checksum
RUN abuild -r
RUN find /home/builder/packages -name '*.apk' -exec cp {} /dist/ \;

CMD ["/bin/sh"]
