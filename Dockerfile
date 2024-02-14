ARG BUILD_IMG
FROM ${BUILD_IMG}
ARG DEBIANDIR
WORKDIR /build/dependencies
COPY ${DEBIANDIR}/control .


RUN apt-get update
RUN mk-build-deps --install --tool='apt-get -o Debug::pkgProblemResolver=yes --no-install-recommends --yes' control
RUN apt-get update
