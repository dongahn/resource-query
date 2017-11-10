#!/bin/sh

test_description='Test Scheduling On Tiny Machine Configuration'

. $(dirname $0)/sharness.sh

cmp_dir="${SHARNESS_TEST_SRCDIR}/data/commands"
exp_dir="${SHARNESS_TEST_SRCDIR}/data/expected"
grugs="${SHARNESS_TEST_SRCDIR}/data/grugs/tiny.graphml"
query="${SHARNESS_TEST_SRCDIR}/../resource-query"

#
# Selection Policy -- High ID first (-P high)
#     The resource vertex with higher ID is preferred among its kind
#     (e.g., node0 is preferred over node1 if available)
#

cmds001="${cmp_dir}/test001.4x.cmds"
test001_desc="match allocate 4 jobspecs with 1 slot: 1 socket: 1 core (pol=hi)"
test_expect_success "${test001_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds001} > 001.R.out &&
    test_cmp 001.R.out ${exp_dir}/tiny/001.R.out
'

cmds002="${cmp_dir}/test001.5x.cmds"
test002_desc="match allocate 5 jobspecs instead - last one must fail (pol=hi)"
test_expect_success "${test002_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds002} > 002.R.out &&
    test_cmp 002.R.out ${exp_dir}/tiny/002.R.out
'

cmds003="${cmp_dir}/test001.10x.cmds"
test003_desc="match allocate_orelse_reserve 10 jobspecs (pol=hi)"
test_expect_success "${test003_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds003} > 003.R.out &&
    test_cmp 003.R.out ${exp_dir}/tiny/003.R.out
'

# Note that the memory pool granularity is 2GB
cmds004="${cmp_dir}/test001.3x.node1.slot1.socket2.c5-g1-m6.cmds"
test004_desc="match allocate 3 jobspecs with 1 slot: 2 sockets (pol=hi)"
test_expect_success "${test004_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds004} > 004.R.out &&
    test_cmp 004.R.out ${exp_dir}/tiny/004.R.out
'

cmds005="${cmp_dir}/test001.100x.node1.slot1.socket2.c5-g1-m6.cmds"
test005_desc="match allocate_orelse_reserve 100 jobspecs instead (pol=hi)"
test_expect_success "${test005_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds005} > 005.R.out &&
    test_cmp 005.R.out ${exp_dir}/tiny/005.R.out
'

cmds006="${cmp_dir}/test001.2x.slot2.node1.cmds"
test006_desc="match allocate 2 jobspecs with 2 nodes - last must fail (pol=hi)"
test_expect_success "${test006_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds006} > 006.R.out &&
    test_cmp 006.R.out ${exp_dir}/tiny/006.R.out
'

cmds007="${cmp_dir}/test001.17x.slot1.core8-memory2.cmds"
test007_desc="match allocate 17 jobspecs with 1 slot (8c,2m) (pol=hi)"
test_expect_success "${test007_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds007} > 007.R.out &&
    test_cmp 007.R.out ${exp_dir}/tiny/007.R.out
'

#
# Selection Policy -- Low ID first (-P low)
#     The resource vertex with lower ID is preferred among its kind
#     (e.g., node1 is preferred over node0 if available)
#

cmds008="${cmp_dir}/test001.4x.cmds"
test008_desc="match allocate 4 jobspecs with 1 slot: 1 socket: 1 core (pol=low)"
test_expect_success "${test008_desc}" '
    ${query} -G ${grugs} -S CA -P low < ${cmds008} > 008.R.out &&
    test_cmp 008.R.out ${exp_dir}/tiny/008.R.out
'

cmds009="${cmp_dir}/test001.5x.cmds"
test009_desc="match allocate 5 jobspecs instead - last one must fail (pol=hi)"
test_expect_success "${test009_desc}" '
    ${query} -G ${grugs} -S CA -P low < ${cmds009} > 009.R.out &&
    test_cmp 009.R.out ${exp_dir}/tiny/009.R.out
'

cmds010="${cmp_dir}/test001.10x.cmds"
test010_desc="attempt to match allocate_orelse_reserve 10 jobspecs (pol=low)"
test_expect_success "${test010_desc}" '
    ${query} -G ${grugs} -S CA -P low < ${cmds010} > 010.R.out &&
    test_cmp 010.R.out ${exp_dir}/tiny/010.R.out
'

# Note that the memory pool granularity is 2GB
cmds011="${cmp_dir}/test001.3x.node1.slot1.socket2.c5-g1-m6.cmds"
test011_desc="match allocate 3 jobspecs with 1 slot: 2 sockets (pol=hi)"
test_expect_success "${test011_desc}" '
    ${query} -G ${grugs} -S CA -P low < ${cmds011} > 011.R.out &&
    test_cmp 011.R.out ${exp_dir}/tiny/011.R.out
'

cmds012="${cmp_dir}/test001.100x.node1.slot1.socket2.c5-g1-m6.cmds"
test012_desc="match allocate_orelse_reserve 100 jobspecs instead (pol=low)"
test_expect_success "${test011_desc}" '
    ${query} -G ${grugs} -S CA -P low < ${cmds012} > 012.R.out &&
    test_cmp 012.R.out ${exp_dir}/tiny/012.R.out
'

cmds013="${cmp_dir}/test001.2x.slot2.node1.cmds"
test013_desc="match allocate 2 jobspecs with 2 nodes - last must fail (pol=low)"
test_expect_success "${test013_desc}" '
    ${query} -G ${grugs} -S CA -P low < ${cmds013} > 013.R.out &&
    test_cmp 013.R.out ${exp_dir}/tiny/013.R.out
'

cmds014="${cmp_dir}/test001.17x.slot1.core8-memory2.cmds"
test014_desc="match allocate 17 jobspecs with 1 slot (8c,2m) (pol=hi)"
test_expect_success "${test014_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds014} > 014.R.out &&
    test_cmp 014.R.out ${exp_dir}/tiny/014.R.out
'

test_done
