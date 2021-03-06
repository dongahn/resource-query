#!/bin/sh

test_description='Test simple power allocation cases'

. $(dirname $0)/sharness.sh

cmd_dir="${SHARNESS_TEST_SRCDIR}/data/commands/simple_power"
exp_dir="${SHARNESS_TEST_SRCDIR}/data/expected/simple_power"
grugs="${SHARNESS_TEST_SRCDIR}/data/grugs/power.graphml"
query="${SHARNESS_TEST_SRCDIR}/../resource-query"

#
# Selection Policy -- High ID first (-P high)
#     The resource vertex with higher ID is preferred among its kind
#     (e.g., node1 is preferred over node0 if available)
#

cmds001="${cmd_dir}/test010.power.cmds"
test001_desc="match allocate with simple power configuration (pol=hi)"
test_expect_success "${test001_desc}" '
    ${query} -G ${grugs} -S PA -P high -t 001.R.out < ${cmds001} &&
    test_cmp 001.R.out ${exp_dir}/001.R.out
'

#
# Selection Policy -- High ID first (-P low)
#     The resource vertex with higher ID is preferred among its kind
#     (e.g., node0 is preferred over node1 if available)
#

cmds002="${cmd_dir}/test010.power.cmds"
test002_desc="match allocate with simple power configuration (pol=low)"
test_expect_success "${test002_desc}" '
    ${query} -G ${grugs} -S PA -P low -t 002.R.out < ${cmds002} &&
    test_cmp 002.R.out ${exp_dir}/002.R.out
'

test_done
