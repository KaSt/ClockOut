FROM registry.access.redhat.com/ubi9/ubi

ARG VERSION=1.0.0
WORKDIR /build

RUN dnf install -y make gcc ncurses-devel json-c-devel rpm-build tar \
    && dnf clean all

COPY . /build/src
RUN mkdir -p /root/rpmbuild/SOURCES /root/rpmbuild/SPECS /dist /tmp/sources/clockout-${VERSION}
RUN cp -r /build/src/* /tmp/sources/clockout-${VERSION}/
RUN tar -czf /root/rpmbuild/SOURCES/clockout-${VERSION}.tar.gz -C /tmp/sources clockout-${VERSION}
RUN cp /build/src/packaging/rpm/clockout.spec /root/rpmbuild/SPECS/clockout.spec

RUN rpmbuild -ba /root/rpmbuild/SPECS/clockout.spec
RUN find /root/rpmbuild/RPMS -name 'clockout-*.rpm' -exec cp {} /dist/ \;

CMD ["/bin/bash"]
