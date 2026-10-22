#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
"${CXX:-g++}" -std=c++20 -O2 -Wall -Wextra -Werror \
    tests/policy_tests.cpp -o build/policy-tests
./build/policy-tests
"${CXX:-g++}" -std=c++20 -O2 -Wall -Wextra -Werror \
    -Itests/fakes tests/adapter_tests.cpp -o build/adapter-tests
./build/adapter-tests
"${CXX:-g++}" -std=c++20 -O2 -Wall -Wextra -Werror \
    -pthread tests/motion_tests.cpp -o build/motion-tests
./build/motion-tests
