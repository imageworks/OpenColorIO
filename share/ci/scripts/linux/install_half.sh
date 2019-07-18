#!/usr/bin/env bash

set -ex

OPENEXR_VERSION="$1"

git clone https://github.com/openexr/openexr.git
cd openexr

if [ "$OPENEXR_VERSION" == "latest" ]; then
    LATEST_TAG=$(git describe --abbrev=0 --tags)
    git checkout tags/${LATEST_TAG} -b ${LATEST_TAG}
else
    git checkout tags/v${OPENEXR_VERSION} -b v${OPENEXR_VERSION}
fi

mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DOPENEXR_BUILD_OPENEXR=OFF \
      -DOPENEXR_BUILD_PYTHON_LIBS=OFF \
      -DOPENEXR_BUILD_SHARED=OFF \
      -DOPENEXR_BUILD_STATIC=ON \
      -DOPENEXR_ENABLE_TESTS=OFF \
      -DCMAKE_C_FLAGS="-fPIC" \
      -DCMAKE_CXX_FLAGS="-fPIC" \
      ../.
make -j4 Half_static
sudo cmake -P IlmBase/Half/cmake_install.cmake

cd ../..
rm -rf openexr
