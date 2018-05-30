# Old enough so that it should work on most supported distrib
FROM ubuntu:xenial

# Install base building packages
RUN apt-get update -q && DEBIAN_FRONTEND=noninteractive apt-get install -q -y --no-install-recommends \
        locales \
        build-essential \
        clang \
        ninja-build \
        cmake \
        extra-cmake-modules \
        libsqlite3-dev \
        libssl-dev \
        libglu1-mesa-dev \
        libx11-xcb-dev \
        software-properties-common \
        git \
        file

#add Qt
RUN add-apt-repository ppa:beineri/opt-qt-5.10.1-xenial && apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -q -y --no-install-recommends qt510base qt510svg qt510tools qt510translations

RUN apt-get clean
RUN locale-gen en_US.UTF-8 && DEBIAN_FRONTEND=noninteractive dpkg-reconfigure locales

# Install linuxdeployqt
ADD https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage /tmp/linuxdeployqt.AppImage
RUN chmod a+x /tmp/linuxdeployqt.AppImage && \
    /tmp/linuxdeployqt.AppImage --appimage-extract && \
    mv squashfs-root /opt/linuxdeployqt && \
    ln -s /opt/linuxdeployqt/AppRun /usr/local/bin/linuxdeployqt && \
    rm /tmp/linuxdeployqt.AppImage

