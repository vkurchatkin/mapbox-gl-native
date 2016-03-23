#!/usr/bin/env bash

set -e
set -o pipefail
set -u

xcodebuild \
    -project ./platform/ios/test/ios-tests.xcodeproj \
    -scheme 'Mapbox GL Tests' \
    -sdk iphonesimulator \
    -destination 'platform=iOS Simulator,name=iPhone 6,OS=latest' \
    test

xcodebuild \
    -project ./build/ios-all/gyp/ios.xcodeproj \
    -scheme 'test' \
    -sdk iphonesimulator \
    -destination 'platform=iOS Simulator,name=iPhone 6,OS=latest' \
    build

ios-sim launch `xcrun simctl get_app_container booted com.mapbox.MapboxGLUnitTest`
