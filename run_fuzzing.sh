#!/usr/bin/env bash
# Copyright 2020 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# This script runs the fuzzing tool with a predefined corpus located at the
# relative path "corpus_tests/test_corpus/", using a black box monitor.
# Command line arguments:
# 1. Path of the device.
# 2. Path of the test corpus.
# 3. Monitor type.
# 4. Connection port for the GDB server, if it's used.

path=_
corpus=corpus_tests/test_corpus/
monitor=blackbox
port=2331

# parse parameters
for arg in "$@"
do
case $arg in
    --token_path=*)
    path="${arg#*=}"
    shift
    ;;
    --corpus_path=*)
    corpus="${arg#*=}"
    shift
    ;;
    --monitor=*)
    monitor="${arg#*=}"
    shift
    ;;
    --port=*)
    port="${arg#*=}"
    shift
    ;;
esac
done

if [ "$corpus" = "corpus_tests/test_corpus/" ]
then
    git submodule init
    git submodule update
fi

bazel run //:corpus_test -- --token_path="$path" --corpus_path="$corpus" --monitor="$monitor" --port="$port"
