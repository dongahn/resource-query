#!/bin/sh

test_description='Test min and max modable matching'

. $(dirname $0)/sharness.sh

cmd_dir="${SHARNESS_TEST_SRCDIR}/data/commands/min_max"
exp_dir="${SHARNESS_TEST_SRCDIR}/data/expected/min_max"
grugs="${SHARNESS_TEST_SRCDIR}/data/grugs/resv_test.graphml"
query="${SHARNESS_TEST_SRCDIR}/../resource-query"

#
# Selection Policy -- High ID first (-P high)
#     The resource vertex with higher ID is preferred among its kind
#     (e.g., node1 is preferred over node0 if available)
#

cmds001="${cmd_dir}/test008.01.minmax.cmds"
test001_desc="min/max with OP=multiplication on slot type works (pol=hi)"
test_expect_success "${test001_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds001} > 001.R.out &&
    test_cmp 001.R.out ${exp_dir}/001.R.out
'

cmds002="${cmd_dir}/test008.02.minmax.cmds"
test002_desc="min/max with OP=addition on slot type works (pol=hi)"
test_expect_success "${test002_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds002} > 002.R.out &&
    test_cmp 002.R.out ${exp_dir}/002.R.out
'

cmds003="${cmd_dir}/test008.03.minmax.cmds"
test003_desc="min/max with OP=multiplication on node type works (pol=hi)"
test_expect_success "${test003_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds003} > 003.R.out &&
    test_cmp 003.R.out ${exp_dir}/003.R.out
'

cmds004="${cmd_dir}/test008.04.minmax.cmds"
test004_desc="min/max with OP=addition on node type works (pol=hi)"
test_expect_success "${test004_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds004} > 004.R.out &&
    test_cmp 004.R.out ${exp_dir}/004.R.out
'

cmds005="${cmd_dir}/test008.05.minmax.cmds"
test005_desc="min/max with OP=power on node type works (pol=hi)"
test_expect_success "${test005_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds005} > 005.R.out &&
    test_cmp 005.R.out ${exp_dir}/005.R.out
'

#
# Selection Policy -- High ID first (-P low)
#     The resource vertex with higher ID is preferred among its kind
#     (e.g., node0 is preferred over node1 if available)
#

cmds006="${cmd_dir}/test008.01.minmax.cmds"
test006_desc="min/max with OP=multiplication on slot type works (pol=low)"
test_expect_success "${test006_desc}" '
    ${query} -G ${grugs} -S CA -P low < ${cmds006} > 006.R.out &&
    test_cmp 006.R.out ${exp_dir}/006.R.out
'

cmds007="${cmd_dir}/test008.02.minmax.cmds"
test007_desc="min/max with OP=addition on slot type works (pol=low)"
test_expect_success "${test007_desc}" '
    ${query} -G ${grugs} -S CA -P low < ${cmds007} > 007.R.out &&
    test_cmp 007.R.out ${exp_dir}/007.R.out
'

cmds008="${cmd_dir}/test008.03.minmax.cmds"
test008_desc="min/max with OP=multiplication on node type works (pol=low)"
test_expect_success "${test008_desc}" '
    ${query} -G ${grugs} -S CA -P low < ${cmds008} > 008.R.out &&
    test_cmp 008.R.out ${exp_dir}/008.R.out
'

cmds009="${cmd_dir}/test008.04.minmax.cmds"
test009_desc="min/max with OP=addition on node type works (pol=low)"
test_expect_success "${test009_desc}" '
    ${query} -G ${grugs} -S CA -P low < ${cmds009} > 009.R.out &&
    test_cmp 009.R.out ${exp_dir}/009.R.out
'

cmds010="${cmd_dir}/test008.05.minmax.cmds"
test010_desc="min/max with OP=power on node type works (pol=low)"
test_expect_success "${test010_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds010} > 010.R.out &&
    test_cmp 010.R.out ${exp_dir}/010.R.out
'

test_done
