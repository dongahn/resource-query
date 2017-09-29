/*****************************************************************************\
 *  Copyright (c) 2014 Lawrence Livermore National Security, LLC.  Produced at
 *  the Lawrence Livermore National Laboratory (cf, AUTHORS, DISCLAIMER.LLNS).
 *  LLNL-CODE-658032 All rights reserved.
 *
 *  This file is part of the Flux resource manager framework.
 *  For details, see https://github.com/flux-framework.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the license, or (at your option)
 *  any later version.
 *
 *  Flux is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the terms and conditions of the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *  See also:  http://www.gnu.org/licenses/
\*****************************************************************************/

#include <iostream>
#include <sstream>
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <map>
#include <vector>
#include <sys/time.h>
#include "tap.h"
#include "planner.h"

const int million = 1048576;

struct perf_t {
    planner_t *planner;
    int64_t span_count;
    std::string label;
    std::string interval;
    double elapse;
};

std::map<std::string, std::vector<perf_t *> > exp_data;

static void to_stream (int64_t base_time, uint64_t duration, const uint64_t *cnts,
                      const char **types, size_t len, std::stringstream &ss)
{
    if (base_time != -1)
        ss << "B(" << base_time << "):";

    ss << "D(" << duration << "):" << "R(<";
    for (unsigned int i = 0; i < len; ++i)
        ss << types[i] << "(" << cnts[i] << ")";

    ss << ">)";
}

static double elapse_time (timeval &st, timeval &et)
{
    double ts1 = (double) st.tv_sec + (double) st.tv_usec/1000000.0f;
    double ts2 = (double) et.tv_sec + (double) et.tv_usec/1000000.0f;
    return ts2 - ts1;
}

int report (std::string experiment)
{
    int i, start;
    std::cout << "Experiment: " << experiment << std::endl;
    std::cout << "Overlap-Factor";
    size_t sz = exp_data.begin ()->second.size ();
    start = million >> (sz - 1);
    //std::cout << "start: " << start;
    for (i = start; i < (million + 1) ; i = i << 1) {
        std::cout << ", " << i;
    }
    std::cout << std::endl;

    for (auto &kv : exp_data) {
        std::cout << kv.first;
        for (auto &perf : kv.second) {
            std::cout << ", " << perf->elapse;
        }
        for (auto &perf : kv.second) {
            delete perf;
        }
        kv.second.clear ();
        std::cout << std::endl;
    }

    if (!exp_data.empty ())
        exp_data.clear ();

    return 0;
}

perf_t *create_perf (planner_t *ctx, int64_t count, int overlap, double elapse, int64_t end_time)
{
    std::stringstream ss;
    perf_t *perf = new perf_t ();
    perf->planner = ctx;
    perf->span_count = count;
    ss << "Overlaps(" << overlap << ")";
    perf->label = ss.str ();
    perf->elapse = elapse;
    ss.str () = "";
    ss << "[0, " << end_time << ")";
    perf->interval = ss.str ();
    return perf;
}

int test_query_perf_1d ()
{
    int64_t i;
    int end = 0, j, rc;
    int64_t at=0, span=0, avail=0;
    size_t len = 1;
    bool bo = false;
    struct timeval st, et;
    const uint64_t resource_totals[] = {10};
    const char *resource_types[] = {"core"};
    std::vector<uint64_t> count_vector;
    std::vector<int64_t> query_times;
    planner_t *ctx = NULL;
    std::stringstream ss;

    count_vector.push_back (10);
    count_vector.push_back (5);
    count_vector.push_back (2);
    count_vector.push_back (1);

    errno = 0;
    to_stream (0, INT64_MAX, resource_totals,
               (const char **)resource_types, len, ss);
    ctx = planner_new (0, INT64_MAX, resource_totals, resource_types, len);
    ok ((ctx && !errno), "new with (%s)", ss.str ().c_str ());
    ss.str ("");

    for (auto count : count_vector) {
        for (end = 2048; end < (2*million + 1); end = end << 1) {
            ctx = planner_new (0, INT64_MAX, resource_totals, resource_types, len);
            int overlap_factor = resource_totals[0]/count;
            std::vector<int64_t> spans;
            for (i = 0; i < end; ++i) {
                at = (int64_t)(i/overlap_factor * 1000);
                span = planner_add_span (ctx, at, 1000, (const uint64_t *)&count, len);
                spans.push_back (span);
                bo = (bo || span == -1);
            }

            for (i = 0; i < end; i += 2) {
                // delete 1/2 of the spans
                rc = planner_rem_span (ctx, (int64_t)spans[i]);
                bo = (bo || rc == -1);
            }
            //fix the seed for reproducibility
            srandom (i);
            for (j = 0; j < 1024; ++j) {
                double norm = (double)random ()/(double)(RAND_MAX);
                double adjusted = norm * (double)(at + 1000);
                int64_t query_time = (int64_t)adjusted;
                query_times.push_back (query_time);
            }

            gettimeofday (&st, NULL);
            for (j = 0; j < 1024; ++j) {
                avail = planner_avail_resources_at (ctx, query_times[j], 0);
                bo = (bo || avail == -1);
            }
            gettimeofday (&et, NULL);
            // Generate a data point
            perf_t *p = create_perf (ctx, planner_span_size (ctx), overlap_factor,
                                     elapse_time (st, et), at + 1000);
            ss.str ("");
            ss << "Overlap(" << overlap_factor << ")";
            exp_data[ss.str ()].push_back (p);
            query_times.clear ();
        }
    }
    ok (!bo && !errno, "time-based query performance: 1 resource type");
    printf ("%s\n", std::strerror (errno));
    return 0;
}

int test_avail_time_perf ()
{
    int64_t i;
    int end = 0, j, rc;
    int64_t at=0, span=0, t=0;
    size_t len = 1;
    bool bo = false;
    struct timeval st, et;
    const uint64_t resource_totals[] = {10};
    const char *resource_types[] = {"core"};
    std::vector<uint64_t> count_vector;
    std::vector<int64_t> query_times;
    planner_t *ctx = NULL;
    std::stringstream ss;

    count_vector.push_back (10);
    count_vector.push_back (5);
    count_vector.push_back (2);
    count_vector.push_back (1);

    errno = 0;
    to_stream (0, INT64_MAX, resource_totals,
               (const char **)resource_types, len, ss);
    ctx = planner_new (0, INT64_MAX, resource_totals, resource_types, len);
    ok ((ctx && !errno), "new with (%s)", ss.str ().c_str ());
    ss.str ("");

    for (auto count : count_vector) {
        //for (end = 32768; end < (2*million + 1); end = end << 1) {
        for (end = 2*million; end < 2*million + 1; end = end << 1) {
            ctx = planner_new (0, INT64_MAX, resource_totals, resource_types, len);
            int overlap_factor = resource_totals[0]/count;
            std::vector<int64_t> spans;
            for (i = 0; i < end; ++i) {
                at = (int64_t)(i/overlap_factor * 1000);
                span = planner_add_span (ctx, at, 1000, (const uint64_t *)&count, len);
                spans.push_back (span);
                bo = (bo || span == -1);
            }

            for (i = 0; i < end; i += 2) {
                // delete 1/2 of the spans
                rc = planner_rem_span (ctx, (int64_t)spans[i]);
                bo = (bo || rc == -1);
            }
            std::cout << "starting now..." << std::endl;
            j = 0;
            t = planner_avail_time_first (ctx, 0+1, 1, (const uint64_t *)&count, len);
            bo = (bo || t == -1);
            gettimeofday (&st, NULL);
            while (j < million/11 && t != -1) {
                t = planner_avail_time_next (ctx);
                bo = (bo || t == -1);
                //if (bo)
                //    std::cout << end << ": " << count << ":" << j << ":" << t << std::endl;
                j++;
            }
            gettimeofday (&et, NULL);
            // Generate a data point
            perf_t *p = create_perf (ctx, planner_span_size (ctx), overlap_factor,
                                     elapse_time (st, et), at + 1000);
            ss.str ("");
            ss << "Overlap(" << overlap_factor << ")";
            exp_data[ss.str ()].push_back (p);
            query_times.clear ();
        }
    }
    ok (!bo && !errno, "resource-based query performance: 1 resource type");
    printf ("%s\n", std::strerror (errno));
    return 0;
}


int test_rem_perf_1d ()
{
    int end = 0, i, rc;
    int64_t at, span;
    size_t len = 1;
    bool bo = false;
    struct timeval st, et;
    const uint64_t resource_totals[] = {10};
    const char *resource_types[] = {"core"};
    std::vector<uint64_t> count_vector;
    std::vector<int64_t> query_times;
    planner_t *ctx = NULL;
    std::stringstream ss;

    count_vector.push_back (10);
    count_vector.push_back (5);
    count_vector.push_back (2);
    count_vector.push_back (1);

    errno = 0;
    to_stream (0, INT64_MAX, resource_totals,
               (const char **)resource_types, len, ss);
    ctx = planner_new (0, INT64_MAX, resource_totals, resource_types, len);
    ok ((ctx && !errno), "new with (%s)", ss.str ().c_str ());
    ss.str ("");

    for (auto count : count_vector) {
        for (end = 1024; end < (million + 1); end = end << 1) {
            ctx = planner_new (0, INT64_MAX, resource_totals, resource_types, len);
            int overlap_factor = resource_totals[0]/count;
            std::vector<int64_t> spans;
            for (i = 0; i < end; ++i) {
                at = i/overlap_factor * 1000;
                span = planner_add_span (ctx, at, 1000, (const uint64_t *)&count, 1);
                spans.push_back (span);
                bo = (bo || span == -1);
            }

            int stride = i / 1024;
            gettimeofday (&st, NULL);
            for (i = 0; i < end; i += stride) {
                // delete 1024 spans spread equally across the interval
                rc = planner_rem_span (ctx, spans[i]);
                bo = (bo || rc == -1);
            }
            gettimeofday (&et, NULL);

            // Generate a data point //TODO
            perf_t *p = create_perf (ctx, end, overlap_factor,
                                     elapse_time (st, et), at + 1000);
            ss.str ("");
            ss << "Overlap(" << overlap_factor << ")";
            exp_data[ss.str ()].push_back (p);
        }
    }
    ok (!bo && !errno, "remove performance: 1 resource type");
    return 0;
}

int test_add_perf_1d ()
{
    int end = 0, i;
    int64_t at, span;
    size_t len = 1;
    bool bo = false;
    struct timeval st, et;
    const uint64_t resource_totals[] = {10};
    const char *resource_types[] = {"core"};
    std::vector<uint64_t> count_vector;
    planner_t *ctx = NULL;
    std::stringstream ss;

    count_vector.push_back (10);
    count_vector.push_back (5);
    count_vector.push_back (2);
    count_vector.push_back (1);

    errno = 0;
    to_stream (0, INT64_MAX, resource_totals,
               (const char **)resource_types, len, ss);
    ctx = planner_new (0, INT64_MAX, resource_totals, resource_types, len);
    ok ((ctx && !errno), "new with (%s)", ss.str ().c_str ());
    ss.str ("");

    for (auto count : count_vector) {
        for (end = 1024; end < (million + 1); end = end << 1) {
            ctx = planner_new (0, INT64_MAX, resource_totals, resource_types, len);
            int overlap_factor = resource_totals[0]/count;
            gettimeofday (&st, NULL);
            for (i = 0; i < end; ++i) {
                at = i/overlap_factor * 1000;
                span = planner_add_span (ctx, at, 1000, (const uint64_t *)&count, 1);
                bo = (bo || span == -1);
            }
            gettimeofday (&et, NULL);

            // Generate a data point
            perf_t *p = create_perf (ctx, planner_span_size (ctx), overlap_factor,
                                     elapse_time (st, et), at + 1000);
            ss.str ("");
            ss << "Overlap(" << overlap_factor << ")";
            exp_data[ss.str ()].push_back (p);
        }
    }
    ok (!bo && !errno, "add performance: 1 resource type");
    return 0;
}

int test_add_perf_5d ()
{
    int end = 0, i;
    int64_t at, span;
    size_t len = 5;
    bool bo = false;
    struct timeval st, et;
    const uint64_t resource_totals[] = {10, 100, 1000, 10000, 100000};
    const char *resource_types[] = {"cluster", "rack", "node", "socket", "core"};
    std::vector<std::vector<uint64_t> > count_vv;
    planner_t *ctx = NULL;
    std::stringstream ss;

    count_vv.push_back (std::vector<uint64_t>());
    count_vv.push_back (std::vector<uint64_t>());
    count_vv.push_back (std::vector<uint64_t>());
    count_vv.push_back (std::vector<uint64_t>());
    for (i = 0; i < 5; ++i) {
        count_vv[0].push_back (resource_totals[i]);
        count_vv[1].push_back (resource_totals[i]/2);
        count_vv[2].push_back (resource_totals[i]/5);
        count_vv[3].push_back (resource_totals[i]/10);
    }

    errno = 0;
    to_stream (0, INT64_MAX, resource_totals,
               (const char **)resource_types, len, ss);
    ctx = planner_new (0, INT64_MAX, resource_totals, resource_types, len);
    ok ((ctx && !errno), "new with (%s)", ss.str ().c_str ());
    ss.str ("");

    for (auto &count_vector : count_vv) {
        for (end = 1024; end < (million + 1); end = end<<1) {
            ctx = planner_new (0, INT64_MAX, resource_totals, resource_types, len);
            int overlap_factor = resource_totals[0]/count_vector[0];
            gettimeofday (&st, NULL);
            for (i = 0; i < end; ++i) {
                at = i/overlap_factor * 1000;
                span = planner_add_span (ctx, at, 1000,
                                         (const uint64_t *)&(count_vector[0]), 5);
                bo = (bo || span == -1);
            }
            gettimeofday (&et, NULL);

            // Generate a data point
            perf_t *p = create_perf (ctx, planner_span_size (ctx), overlap_factor,
                                     elapse_time (st, et), at + 1000);

            ss.str ("");
            ss << "Overlap(" << overlap_factor << ")";
            exp_data[ss.str ()].push_back (p);
        }
    }
    ok (!bo && !errno, "add performance: 5 resource types");
    return 0;
}

int test_add_performance ()
{
    test_add_perf_1d ();
    return report ("Planner Add Performance: 1D");
}

int test_add_performance2 ()
{
    test_add_perf_5d ();
    return report ("Planner Add Performance: 5D");
}

int test_query_performance ()
{
    test_query_perf_1d ();
    return report ("Planner Time-Based Query Performance");
}

int test_avail_time_performance ()
{
    test_avail_time_perf ();
    return report ("Planner Resource_Based Query Performance");
}

int test_rem_performance ()
{
    test_rem_perf_1d ();
    return report ("Planner Remove Performance");
}

int main (int argc, char *argv[])
{
    plan (10);

    //test_add_performance ();

    //test_add_performance2 ();

    //test_query_performance ();

    test_avail_time_performance ();

    //test_rem_performance ();

    done_testing ();

    return EXIT_SUCCESS;
}

/*
 * vi: ts=4 sw=4 expandtab
 */
