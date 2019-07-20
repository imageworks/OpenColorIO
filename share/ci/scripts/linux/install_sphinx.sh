#!/usr/bin/env bash

set -ex

if [[ "$(python -c 'import sys; print(sys.version_info[0])')" == "2" ]]; then
    # Python 2
    SPHINX_VERSION="$1"
else
    # Python 3
    SPHINX_VERSION="$2"
fi

if [ "$SPHINX_VERSION" == "latest" ]; then
    sudo pip install Sphinx
else
    sudo pip install Sphinx==${SPHINX_VERSION}
fi
