#!/usr/bin/env bash

set -e
set -o pipefail

if [[ ${TRAVIS_OS_NAME} == "linux" ]]; then
    source ./platform/glfw/scripts/setup.sh
else
    source ./platform/${TRAVIS_OS_NAME}/scripts/setup.sh
fi

BUILDTYPE=${BUILDTYPE:-Release}

################################################################################
# Build
################################################################################

source ~/.nvm/nvm.sh
nvm use $NODE_VERSION

mapbox_time "compile_program" \
npm install --build-from-source

################################################################################
# Test
################################################################################

# Travis OS X has no GPU
if [[ ${TRAVIS_OS_NAME} == "linux" ]]; then
    mapbox_time "run_tests" \
    npm test

    mapbox_time "run_render_tests" \
    npm run test-suite
fi
