FROM archlinux:base

ARG VERSION=1.0.0

RUN pacman -Syu --noconfirm \
    && pacman -S --noconfirm base-devel ncurses json-c \
    && pacman -Scc --noconfirm

RUN useradd -m build
# makepkg requires a non-root user

WORKDIR /build
COPY . /build/src
RUN mkdir -p /dist
RUN chown -R build:build /build /dist

USER build
RUN mkdir -p /build/pkgbuild /build/sources/clockout-${VERSION}
RUN cp -r /build/src/* /build/sources/clockout-${VERSION}/
RUN tar -czf /build/pkgbuild/clockout-${VERSION}.tar.gz -C /build/sources clockout-${VERSION}
RUN sed -e "s/@VERSION@/${VERSION}/g" /build/src/packaging/arch/PKGBUILD > /build/pkgbuild/PKGBUILD
WORKDIR /build/pkgbuild
RUN makepkg --noconfirm --clean --cleanbuild --noprepare
RUN mv clockout-*.pkg.tar.zst /dist/

CMD ["/bin/bash"]
