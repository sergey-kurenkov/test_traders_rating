#!/usr/bin/env bash
if (($# == 0)); then
	echo "Usage: $0 user command with args"
	exit 0
fi

if ! command -v lcov; then
	echo "Error, lcov is not found"
	exit 1
fi

if ! command -v genhtml; then
	echo "Error, genhtml is not found"
	exit 1
fi

GCOV_OPTION="--gcov-tool ""$PWD/llvm-gcov.sh"

lcov $GCOV_OPTION -c -i -d $PWD -b $(dirname $PWD) --no-external -o app_base.info
if (($? != 0)); then
	echo "Error, lcov, initialization"
	exit 1
fi

eval "$@"
if (($? != 0)); then
	echo "Error, application"
	exit 1
fi

lcov $GCOV_OPTION -c -d $PWD -b $(dirname $PWD) --no-external -o app_test.info
if (($? != 0)); then
	echo "Error, lcov, after test"
	exit 1
fi

lcov $GCOV_OPTION -d $PWD -b $(dirname $PWD) -a app_base.info -a app_test.info -o app_total.info
if (($? != 0)); then
	echo "Error, lcov, aggregation"
	exit 1
fi

lcov --remove app_total.info 'benchmark/*' 'gtest/*' 'perftests/*' 'unittests/*' -o app_total_short.info
if (($? != 0)); then
	echo "Error, lcov, remove"
	exit 1
fi

genhtml -o ./coverage-report/ app_total_short.info
if (($? != 0)); then
	echo "Error, genhtml"
	exit 1
fi
