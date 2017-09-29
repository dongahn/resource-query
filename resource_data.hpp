/*****************************************************************************\
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

#ifndef RESOURCE_DATA_HPP
#define RESOURCE_DATA_HPP

#include <uuid/uuid.h>
#include <string>
#include <map>
#include "planner/planner.h"

namespace resource_model {

typedef std::string subsystem_t;
typedef std::map<subsystem_t, std::string> multi_subsystems_t;
typedef std::map<subsystem_t, std::set<std::string> > multi_subsystemsS;

struct color_t {
    enum color_offset_t {
        WHITE_OFFSET = 0,
        GRAY_OFFSET = 1,
        BLACK_OFFSET = 2,
        NEW_BASE = 3
    };

    uint64_t reset (uint64_t color_base)
    {
        return color_base + (uint64_t)NEW_BASE;
    }
    bool white (uint64_t c, uint64_t color_base)
    {
        return c <= (color_base + (uint64_t)WHITE_OFFSET);
    }
    uint64_t white (uint64_t color_base)
    {
        return color_base + (uint64_t)WHITE_OFFSET;
    }
    bool gray (uint64_t c, uint64_t color_base)
    {
        return c == (color_base + (uint64_t)GRAY_OFFSET);
    }
    uint64_t gray (uint64_t color_base)
    {
        return color_base + (uint64_t)GRAY_OFFSET;
    }
    bool black (uint64_t c, uint64_t color_base)
    {
        return c == (color_base + (uint64_t)BLACK_OFFSET);
    }
    uint64_t black (uint64_t color_base)
    {
        return color_base + (uint64_t)BLACK_OFFSET;
    }
};

//! Type to keep track of current schedule state
struct schedule_t {
    schedule_t () { }
    schedule_t (const schedule_t &o)
    {
        tags = o.tags;
        allocations = o.allocations;
        reservations = o.reservations;
        plans = o.plans;
#if 0
        if (o.plans)
            plans = planner_copy (o.plans);
#endif
    }
    schedule_t &operator= (const schedule_t &o)
    {
        tags = o.tags;
        allocations = o.allocations;
        reservations = o.reservations;
        plans = o.plans;

#if 0
        if (o.plans)
            plans = planner_copy (o.plans);
#endif
        return *this;
    }

    std::map<int64_t, int64_t> tags;
    std::map<int64_t, int64_t> allocations;
    std::map<int64_t, int64_t> reservations;
    planner_t *plans = NULL;
};

/*! Base type to organize the data supporting scheduling infrastructure's
 * operations (e.g., graph organization, coloring and edge evaluation).
 */
struct infra_base_t {
    infra_base_t () { }
    infra_base_t (const infra_base_t &o)
    {
        member_of = o.member_of;
    }
    infra_base_t &operator= (const infra_base_t &o)
    {
        member_of = o.member_of;
        return *this;
    }
    virtual ~infra_base_t () { }
    virtual void scrub () = 0;

    multi_subsystems_t member_of;
};

struct pool_infra_t : public infra_base_t {
    pool_infra_t () { }
    pool_infra_t (const pool_infra_t &o): infra_base_t (o)
    {
        job2span = o.job2span;
        //subplans = o.subplans;
        colors = o.colors;
    }
    pool_infra_t &operator= (const pool_infra_t &o)
    {
        infra_base_t::operator= (o);
        job2span = o.job2span;
        subplans = o.subplans;
        colors = o.colors;
#if 0
        for (auto &kv : o.subplans)
            if (kv.second)
                subplans[kv.first] = planner_copy (kv.second);
#endif

        return *this;
    }
    virtual ~pool_infra_t ()
    {
        job2span.clear ();
        for (auto &kv : subplans)
            planner_destroy (&(kv.second));
        colors.clear ();
    }
    virtual void scrub ()
    {
        job2span.clear ();
        for (auto &kv : subplans)
            planner_destroy (&(kv.second));
        colors.clear ();
    }

    std::map<int64_t, int64_t> job2span;
    std::map<subsystem_t, planner_t *> subplans;
    std::map<subsystem_t, uint64_t> colors;
};

struct relation_infra_t : public infra_base_t {
    relation_infra_t () { }
    relation_infra_t (const relation_infra_t &o): infra_base_t (o)
    {
        needs = o.needs;
        best_k_cnt = o.best_k_cnt;
        exclusive = o.exclusive;
    }
    relation_infra_t &operator= (const relation_infra_t &o)
    {
        infra_base_t::operator= (o);
        needs = o.needs;
        best_k_cnt = o.best_k_cnt;
        exclusive = o.exclusive;
        return *this;
    }
    virtual ~relation_infra_t ()
    {

    }
    virtual void scrub ()
    {
        needs = 0;
        best_k_cnt = 0;
        exclusive = 0;
    }

    uint64_t needs = 0;
    uint64_t best_k_cnt = 0;
    int exclusive = 0;
};

//! Resource pool data type
struct resource_pool_t {
    resource_pool_t () { }
    resource_pool_t (const resource_pool_t &o)
    {
        type = o.type;
        paths = o.paths;
        basename = o.basename;
        name = o.name;
        properties = o.properties;
        id = o.id;
        memcpy (uuid, o.uuid, sizeof (uuid));
        size = o.size;
        unit = o.unit;
        schedule = o.schedule;
        idata = o.idata;
    }
    resource_pool_t &operator= (const resource_pool_t &o)
    {
        type = o.type;
        paths = o.paths;
        basename = o.basename;
        name = o.name;
        properties = o.properties;
        id = o.id;
        memcpy (uuid, o.uuid, sizeof (uuid));
        size = o.size;
        unit = o.unit;
        schedule = o.schedule;
        idata = o.idata;
        return *this;
    }
    ~resource_pool_t ()
    {
        paths.clear ();
        properties.clear ();
    }

    // Resource pool data
    std::string type;
    std::map<std::string, std::string> paths;
    std::string basename;
    std::string name;
    std::map<std::string, std::string> properties;
    int64_t id = -1;
    uuid_t uuid;
    unsigned int size = 0;
    std::string unit;

    schedule_t schedule;    //!< schedule data
    pool_infra_t idata;     //!< scheduling infrastructure data
};

/*! Resource relationship type.
 *  An edge is annotated with a set of {key: subsystem, val: relationship}.
 *  An edge can represent a relationship within a subsystem and do this
 *  for multiple subsystems.  However, it cannot represent multiple
 *  relationship within the same subsystem.
 */
struct resource_relation_t {
    resource_relation_t () { }
    resource_relation_t (const resource_relation_t &o)
    {
        name = o.name;
        idata = o.idata;
    }
    resource_relation_t &operator= (const resource_relation_t &o)
    {
        name = o.name;
        idata = o.idata;
        return *this;
    }
    ~resource_relation_t () { }

    std::string name;
    relation_infra_t idata; //!< scheduling infrastructure data
};

} // namespace resource_model

#endif // RESOURCE_DATA_HPP

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
