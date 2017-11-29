#!/bin/bash

if test "$#" -ne 2; then
    echo "Usage: gen_test009.sh output_filename num_job_bundles"
    echo $#
    exit 1
fi

filename=$1
bundle_count=$2

rm $filename

cmd1="match allocate_orelse_reserve ../data/jobspecs/many_jobs/test.01.1node.yaml"
cmd2="match allocate_orelse_reserve ../data/jobspecs/many_jobs/test.01.1node.yaml"
cmd3="match allocate_orelse_reserve ../data/jobspecs/many_jobs/test.01.1node.yaml"
cmd4="match allocate_orelse_reserve ../data/jobspecs/many_jobs/test.01.1node.yaml"
cmd5="match allocate_orelse_reserve ../data/jobspecs/many_jobs/test.02.4node.yaml"
cmd6="match allocate_orelse_reserve ../data/jobspecs/many_jobs/test.02.4node.yaml"
cmd7="match allocate_orelse_reserve ../data/jobspecs/many_jobs/test.03.8node.yaml"
cmd8="match allocate_orelse_reserve ../data/jobspecs/many_jobs/test.03.8node.yaml"
cmd9="match allocate_orelse_reserve ../data/jobspecs/many_jobs/test.04.16node.yaml"
cmd10="match allocate_orelse_reserve ../data/jobspecs/many_jobs/test.06.64node.yaml"

for i in `seq 1 $bundle_count`; do
    echo $cmd1 >> $filename
    echo $cmd2 >> $filename
    echo $cmd3 >> $filename
    echo $cmd4 >> $filename
    echo $cmd5 >> $filename
    echo $cmd6 >> $filename
    echo $cmd7 >> $filename
    echo $cmd8 >> $filename
    echo $cmd9 >> $filename
    echo $cmd10 >> $filename
    job_count=$(($i * 10)) 
    echo "# So far $job_count jobs" >> $filename
    echo "" >> $filename
done

echo "quit" >> $filename
