#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

PYBIND11_VERSION="$1"

git clone https://github.com/pybind/pybind11.git
cd pybind11

if [ "$PYBIND11_VERSION" == "latest" ]; then
    LATEST_TAG=$(git describe --abbrev=0 --tags)
    git checkout tags/${LATEST_TAG} -b ${LATEST_TAG}
else
    git checkout tags/v${PYBIND11_VERSION} -b v${PYBIND11_VERSION}
fi

mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DPYBIND11_INSTALL:BOOL=ON \
      -DPYBIND11_TEST:BOOL=OFF \
      ../.
make -j4
sudo make install

cd ../..
rm -rf pybind11
