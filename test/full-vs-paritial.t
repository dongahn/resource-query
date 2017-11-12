#!/bin/sh

test_description='Test Scheduling On Medium Machine Configuration'

. $(dirname $0)/sharness.sh

cmd_dir="${SHARNESS_TEST_SRCDIR}/data/commands/full-vs-partial"
exp_dir="${SHARNESS_TEST_SRCDIR}/data/expected/full-vs-partial"
grugs="${SHARNESS_TEST_SRCDIR}/data/grugs/medium.graphml"
query="${SHARNESS_TEST_SRCDIR}/../resource-query"

#
# Selection Policy -- High ID first (-P high)
#     The resource vertex with higher ID is preferred among its kind
#     (e.g., node1 is preferred over node0 if available)
#

cmds001="${cmd_dir}/test002.fullspec.cmds"
test001_desc="match allocate with fully specified (pol=hi)"
test_expect_success "${test001_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds001} > 001.R.out &&
    test_cmp 001.R.out ${exp_dir}/001.R.out
'

cmds002="${cmd_dir}/test002.partial-from-rack"
test002_desc="match allocate with partially specified from rack (pol=hi)"
test_expect_success "${test002_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds002} > 002.R.out &&
    test_cmp 002.R.out ${exp_dir}/002.R.out
'

cmds003="${cmd_dir}/test002.partial-from-node"
test003_desc="match allocate with partially specified from node (pol=hi)"
test_expect_success "${test003_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds003} > 003.R.out &&
    test_cmp 003.R.out ${exp_dir}/003.R.out
'

#
# Selection Policy -- High ID first (-P low)
#     The resource vertex with higher ID is preferred among its kind
#     (e.g., node0 is preferred over node1 if available)
#

cmds004="${cmd_dir}/test002.fullspec.cmds"
test004_desc="match allocate with fully specified (pol=low)"
test_expect_success "${test004_desc}" '
    ${query} -G ${grugs} -S CA -P low < ${cmds004} > 004.R.out &&
    test_cmp 004.R.out ${exp_dir}/004.R.out
'

cmds005="${cmd_dir}/test002.partial-from-rack"
test005_desc="match allocate with partially specified from rack (pol=low)"
test_expect_success "${test005_desc}" '
    ${query} -G ${grugs} -S CA -P low < ${cmds005} > 005.R.out &&
    test_cmp 005.R.out ${exp_dir}/005.R.out
'

cmds006="${cmd_dir}/test002.partial-from-node"
test006_desc="match allocate with partially specified from node (pol=low)"
test_expect_success "${test006_desc}" '
    ${query} -G ${grugs} -S CA -P low < ${cmds006} > 006.R.out &&
    test_cmp 006.R.out ${exp_dir}/006.R.out
'

test_done
