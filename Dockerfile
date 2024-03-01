ARG BUILD_IMG
FROM ${BUILD_IMG}
WORKDIR /build/dependencies
COPY CMakeLists.txt CMakeLists.txt
COPY debian_files debian_files 
COPY src src
COPY etc etc
COPY share share
RUN metno-debuild --stopMeFast

RUN mk-build-deps --install --tool='apt-get -o Debug::pkgProblemResolver=yes --no-install-recommends --yes' debian/control
RUN apt-get update

WORKDIR /build
