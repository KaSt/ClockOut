FROM slackware/slackware:15.0

ARG VERSION=1.0.0
WORKDIR /build/src

COPY . /build/src
RUN chmod +x packaging/slackware/clockout.SlackBuild
RUN VERSION=${VERSION} ./packaging/slackware/clockout.SlackBuild
RUN mkdir -p /dist && mv slackware/*.txz /dist/

CMD ["/bin/sh"]
