#!/bin/bash

### Setup ###

splitcode="./src/splitcode"
test_dir="./func_tests"

cmdexec() {
	cmd="$1"
	expect_fail="$2"
	printf "$cmd\n"
	if eval "$cmd 2> /dev/null 1> /dev/null"; then
		if [ "$expect_fail" = "1" ]; then
			printf "^[Failed - Returned normally but expected failure]\n"
			exit 1
		else
			printf "^[OK]\n"
		fi
	else
		if [ "$expect_fail" = "1" ]; then
			printf "^[OK - Exited non-zero as expected]\n"
		else
			printf "^[Failed]\n"
			exit 1
		fi
	fi
}

checkcmdoutput() {
	cmd="$1"
	correct_md5="$2"
	printf "$cmd\n"
	output_md5=$(eval "$cmd"|md5sum|awk '{ print $1 }')
	if [ "$output_md5" = "$correct_md5" ]; then
		printf "^[Output OK]\n"
	else
		printf "^[Output incorrect! Expected: "
		printf "$correct_md5"
		printf " Actual: "
		printf "$output_md5"
		printf "]\n"
		exit 1
	fi	
}

echo "@read0
AAGCTTCCGG
+
KI;<)(,%#$
@read1
AAGCTTCCGG
+
I;<*,(,%#$" > $test_dir/test.fq


# Adapter trimming tests

echo "0.35"
$splitcode --trim-only -b CCAAA --partial5=3:0.35 --left=1 --pipe $test_dir/test.fq
echo "0.33"
$splitcode --trim-only -b CCAAA --partial5=3:0.33 --left=1 --pipe $test_dir/test.fq

checkcmdoutput "$splitcode --trim-only -b CCAAA --partial5=3:0.35 --left=1 --pipe $test_dir/test.fq" b637fbabe71eb90bb9b3399a17eabef7
