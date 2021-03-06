#!/bin/sh

test_description='Test Scheduling On Tiny Machine Configuration'

. $(dirname $0)/sharness.sh

cmd_dir="${SHARNESS_TEST_SRCDIR}/data/commands/basics"
exp_dir="${SHARNESS_TEST_SRCDIR}/data/expected/basics"
grugs="${SHARNESS_TEST_SRCDIR}/data/grugs/tiny.graphml"
query="${SHARNESS_TEST_SRCDIR}/../resource-query"

#
# Selection Policy -- High ID first (-P high)
#     The resource vertex with higher ID is preferred among its kind
#     (e.g., node1 is preferred over node0 if available)
#

cmds001="${cmd_dir}/test001.4x.cmds"
test001_desc="match allocate 4 jobspecs with 1 slot: 1 socket: 1 core (pol=hi)"
test_expect_success "${test001_desc}" '
    ${query} -G ${grugs} -S CA -P high -t 001.R.out < ${cmds001} &&
    test_cmp 001.R.out ${exp_dir}/001.R.out
'

cmds002="${cmd_dir}/test001.5x.cmds"
test002_desc="match allocate 5 jobspecs instead - last one must fail (pol=hi)"
test_expect_success "${test002_desc}" '
    ${query} -G ${grugs} -S CA -P high -t 002.R.out < ${cmds002} &&
    test_cmp 002.R.out ${exp_dir}/002.R.out
'

cmds003="${cmd_dir}/test001.10x.cmds"
test003_desc="match allocate_orelse_reserve 10 jobspecs (pol=hi)"
test_expect_success "${test003_desc}" '
    ${query} -G ${grugs} -S CA -P high -t 003.R.out < ${cmds003} &&
    test_cmp 003.R.out ${exp_dir}/003.R.out
'

# Note that the memory pool granularity is 2GB
cmds004="${cmd_dir}/test001.3x.node1.slot1.socket2.c5-g1-m6.cmds"
test004_desc="match allocate 3 jobspecs with 1 slot: 2 sockets (pol=hi)"
test_expect_success "${test004_desc}" '
    ${query} -G ${grugs} -S CA -P high -t 004.R.out < ${cmds004} &&
    test_cmp 004.R.out ${exp_dir}/004.R.out
'

cmds005="${cmd_dir}/test001.100x.node1.slot1.socket2.c5-g1-m6.cmds"
test005_desc="match allocate_orelse_reserve 100 jobspecs instead (pol=hi)"
test_expect_success "${test005_desc}" '
    ${query} -G ${grugs} -S CA -P high -t 005.R.out < ${cmds005} &&
    test_cmp 005.R.out ${exp_dir}/005.R.out
'

cmds006="${cmd_dir}/test001.2x.slot2.node1.cmds"
test006_desc="match allocate 2 jobspecs with 2 nodes - last must fail (pol=hi)"
test_expect_success "${test006_desc}" '
    ${query} -G ${grugs} -S CA -P high -t 006.R.out < ${cmds006} &&
    test_cmp 006.R.out ${exp_dir}/006.R.out
'

cmds007="${cmd_dir}/test001.9x.slot1.core8-memory2.cmds"
test007_desc="match allocate 9 jobspecs with 1 slot (8c,2m) (pol=hi)"
test_expect_success "${test007_desc}" '
    ${query} -G ${grugs} -S CA -P high -t 007.R.out < ${cmds007} &&
    test_cmp 007.R.out ${exp_dir}/007.R.out
'

cmds008="${cmd_dir}/test001.satisfiability.cmds"
test008_desc="36 core (satisfiable) cores and 37 cores (unsatisfiable) (pol=hi)"
test_expect_success "${test008_desc}" '
    ${query} -G ${grugs} -S CA -P high -t 008.R.out < ${cmds008} &&
    test_cmp 008.R.out ${exp_dir}/008.R.out
'

#
# Selection Policy -- Low ID first (-P low)
#     The resource vertex with lower ID is preferred among its kind
#     (e.g., node0 is preferred over node1 if available)
#

cmds009="${cmd_dir}/test001.4x.cmds"
test009_desc="match allocate 4 jobspecs with 1 slot: 1 socket: 1 core (pol=low)"
test_expect_success "${test009_desc}" '
    ${query} -G ${grugs} -S CA -P low -t 009.R.out < ${cmds009} &&
    test_cmp 009.R.out ${exp_dir}/009.R.out
'

cmds010="${cmd_dir}/test001.5x.cmds"
test010_desc="match allocate 5 jobspecs instead - last one must fail (pol=low)"
test_expect_success "${test010_desc}" '
    ${query} -G ${grugs} -S CA -P low -t 010.R.out < ${cmds010} &&
    test_cmp 010.R.out ${exp_dir}/010.R.out
'

cmds011="${cmd_dir}/test001.10x.cmds"
test011_desc="attempt to match allocate_orelse_reserve 10 jobspecs (pol=low)"
test_expect_success "${test011_desc}" '
    ${query} -G ${grugs} -S CA -P low -t 011.R.out < ${cmds011} &&
    test_cmp 011.R.out ${exp_dir}/011.R.out
'

# Note that the memory pool granularity is 2GB
cmds012="${cmd_dir}/test001.3x.node1.slot1.socket2.c5-g1-m6.cmds"
test012_desc="match allocate 3 jobspecs with 1 slot: 2 sockets (pol=low)"
test_expect_success "${test012_desc}" '
    ${query} -G ${grugs} -S CA -P low -t 012.R.out < ${cmds012} &&
    test_cmp 012.R.out ${exp_dir}/012.R.out
'

cmds013="${cmd_dir}/test001.100x.node1.slot1.socket2.c5-g1-m6.cmds"
test013_desc="match allocate_orelse_reserve 100 jobspecs instead (pol=low)"
test_expect_success "${test013_desc}" '
    ${query} -G ${grugs} -S CA -P low -t 013.R.out < ${cmds013} &&
    test_cmp 013.R.out ${exp_dir}/013.R.out
'

cmds014="${cmd_dir}/test001.2x.slot2.node1.cmds"
test014_desc="match allocate 2 jobspecs with 2 nodes - last must fail (pol=low)"
test_expect_success "${test014_desc}" '
    ${query} -G ${grugs} -S CA -P low -t 014.R.out < ${cmds014} &&
    test_cmp 014.R.out ${exp_dir}/014.R.out
'

cmds015="${cmd_dir}/test001.9x.slot1.core8-memory2.cmds"
test015_desc="match allocate 9 jobspecs with 1 slot (8c,2m) (pol=low)"
test_expect_success "${test015_desc}" '
    ${query} -G ${grugs} -S CA -P low -t 015.R.out < ${cmds015} &&
    test_cmp 015.R.out ${exp_dir}/015.R.out
'

cmds016="${cmd_dir}/test001.satisfiability.cmds"
test016_desc="36 core (satisfiable) cores and 37 cores (unsatisfiable) (pol=low)"
test_expect_success "${test016_desc}" '
    ${query} -G ${grugs} -S CA -P low -t 016.R.out < ${cmds016} &&
    test_cmp 016.R.out ${exp_dir}/016.R.out
'

test_done
