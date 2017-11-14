#!/bin/sh

test_description='Test more global (rack) constraints'

. $(dirname $0)/sharness.sh

cmd_dir="${SHARNESS_TEST_SRCDIR}/data/commands/exclusive"
exp_dir="${SHARNESS_TEST_SRCDIR}/data/expected/exclusive"
grugs="${SHARNESS_TEST_SRCDIR}/data/grugs/medium.graphml"
query="${SHARNESS_TEST_SRCDIR}/../resource-query"

#
# Selection Policy -- High ID first (-P high)
#     The resource vertex with higher ID is preferred among its kind
#     (e.g., node1 is preferred over node0 if available)
#

cmds001="${cmd_dir}/test004.cluster1x-then.cmds"
test001_desc="allocate entire cluster then nothing scheduled (pol=hi)"
test_expect_success "${test001_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds001} > 001.R.out &&
    test_cmp 001.R.out ${exp_dir}/001.R.out
'

cmds002="${cmd_dir}/test004.flat-then-rack4x.cmd"
test002_desc="allocate 1 node then rack4x shouldn't match (pol=hi)"
test_expect_success "${test002_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds002} > 002.R.out &&
    test_cmp 002.R.out ${exp_dir}/002.R.out
'

cmds003="${cmd_dir}/test004.rack-exclusive.cmds"
test003_desc="match allocate with several rack exclusives 1 (pol=hi)"
test_expect_success "${test003_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds003} > 003.R.out &&
    test_cmp 003.R.out ${exp_dir}/003.R.out
'

cmds004="${cmd_dir}/test004.rack1x-full.cmds"
test004_desc="match allocate 4 full rack exclusives (pol=hi)"
test_expect_success "${test004_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds004} > 004.R.out &&
    test_cmp 004.R.out ${exp_dir}/004.R.out
'

cmds005="${cmd_dir}/test004.rack1x.cmds"
test005_desc="match allocate with several rack exclusives 2 (pol=hi)"
test_expect_success "${test005_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds005} > 005.R.out &&
    test_cmp 005.R.out ${exp_dir}/005.R.out
'

cmds006="${cmd_dir}/test004.rack4x-then.cmds"
test006_desc="match allocate 4 rack exclusively then nothing matched 1 (pol=hi)"
test_expect_success "${test006_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds006} > 006.R.out &&
    test_cmp 006.R.out ${exp_dir}/006.R.out
'

cmds007="${cmd_dir}/test004.rackx-then.cmds"
test007_desc="match allocate 4 rack exclusively then nothing matched 2 (pol=hi)"
test_expect_success "${test007_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds007} > 007.R.out &&
    test_cmp 007.R.out ${exp_dir}/007.R.out
'


#
# Selection Policy -- High ID first (-P low)
#     The resource vertex with higher ID is preferred among its kind
#     (e.g., node0 is preferred over node1 if available)
#

cmds008="${cmd_dir}/test004.cluster1x-then.cmds"
test008_desc="allocate entire cluster then nothing scheduled (pol=low)"
test_expect_success "${test008_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds008} > 008.R.out &&
    test_cmp 008.R.out ${exp_dir}/008.R.out
'

cmds009="${cmd_dir}/test004.flat-then-rack4x.cmd"
test009_desc="allocate 1 node then rack4x shouldn't match (pol=low)"
test_expect_success "${test009_desc}" '
    ${query} -G ${grugs} -S CA -P low < ${cmds009} > 009.R.out &&
    test_cmp 009.R.out ${exp_dir}/009.R.out
'

cmds010="${cmd_dir}/test004.rack-exclusive.cmds"
test010_desc="match allocate with several rack exclusives 1 (pol=low)"
test_expect_success "${test010_desc}" '
    ${query} -G ${grugs} -S CA -P low < ${cmds010} > 010.R.out &&
    test_cmp 010.R.out ${exp_dir}/010.R.out
'

cmds011="${cmd_dir}/test004.rack1x-full.cmds"
test011_desc="match allocate 4 full rack exclusives (pol=low)"
test_expect_success "${test011_desc}" '
    ${query} -G ${grugs} -S CA -P low < ${cmds011} > 011.R.out &&
    test_cmp 011.R.out ${exp_dir}/011.R.out
'

cmds012="${cmd_dir}/test004.rack1x.cmds"
test012_desc="match allocate with several rack exclusives 2 (pol=low)"
test_expect_success "${test012_desc}" '
    ${query} -G ${grugs} -S CA -P low < ${cmds012} > 012.R.out &&
    test_cmp 012.R.out ${exp_dir}/012.R.out
'

cmds013="${cmd_dir}/test004.rack4x-then.cmds"
test013_desc="allocate 4 rack exclusively then nothing matched 1 (pol=low)"
test_expect_success "${test013_desc}" '
    ${query} -G ${grugs} -S CA -P low < ${cmds013} > 013.R.out &&
    test_cmp 013.R.out ${exp_dir}/013.R.out
'

cmds014="${cmd_dir}/test004.rackx-then.cmds"
test014_desc="allocate 4 rack exclusively then nothing matched 2 (pol=low)"
test_expect_success "${test014_desc}" '
    ${query} -G ${grugs} -S CA -P low < ${cmds014} > 014.R.out &&
    test_cmp 014.R.out ${exp_dir}/014.R.out
'


test_done
