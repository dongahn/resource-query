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

// TODO: need support for multiple resources the same type at a same level
//       rack[1]->node[2]->socket[1]
//              ->node[1]->socket[2]->gpu[1]
// TODO: need slot support

#ifndef DFU_TRAVERSE_HPP
#define DFU_TRAVERSE_HPP

#include <iostream>
#include <cstdlib>
#include "resource_data.hpp"
#include "resource_graph.hpp"
#include "dfu_match_cb.hpp"
#include "system_defaults.hpp"
#include "scoring_api.hpp"
#include "jobspec.hpp"
#include "planner/planner.h"

namespace resource_model {

namespace detail {

enum visit_t { DFV, UPV };

struct jobmeta_t {
    bool allocate = true;
    int64_t jobid = -1;
    int64_t at = -1;
    uint64_t duration = SYSTEM_DEFAULT_DURATION;

    void build (Jobspec &jspec, bool alloc, int64_t id, int64_t t)
    {
        at = t;
        jobid = id;
        allocate = alloc;
        std::string system_key = "system";
        std::string duration_key = "duration";
        auto i = jspec.attributes.find (system_key);
        if (i->second.find (duration_key) != i->second.end ()) {
            auto j = i->second.find (duration_key);
            duration = (uint64_t)std::atoll (j->second.c_str ());
        }
    }
};

/*! dfu_traverser_t implementation
 */
class dfu_impl_t {
public:
    dfu_impl_t ();
    dfu_impl_t (f_resource_graph_t *g, dfu_match_cb_t *m,
                std::map<subsystem_t, vtx_t> *roots);
    dfu_impl_t (const dfu_impl_t &o);
    dfu_impl_t &operator= (const dfu_impl_t &o);
    ~dfu_impl_t ();

    const f_resource_graph_t *get_graph () const;
    const std::map<subsystem_t, vtx_t> *get_roots () const;
    const dfu_match_cb_t *get_match_cb () const;

    void set_graph (f_resource_graph_t *g);
    void set_roots (std::map<subsystem_t, vtx_t> *roots);
    void set_match_cb (dfu_match_cb_t *m);

    bool exclusivity (const std::vector<Resource> &resources, vtx_t u);

    int prime (const subsystem_t &s, vtx_t u,
               std::map<std::string, int64_t> &to_parent);
    int prime (std::vector<Resource> &resources,
               std::unordered_map<std::string, int64_t> &to_parent);

    int count (planner_t *plan, const std::map<std::string, int64_t> &lookup,
               std::vector<uint64_t> &agg);
    int count (planner_t *plan,
               const std::unordered_map<std::string, int64_t> &lookup,
               std::vector<uint64_t> &agg);
    int select (Jobspec &jspec, vtx_t root, jobmeta_t &meta, bool exclusive,
                unsigned int *needs);
    int update (vtx_t root, jobmeta_t &meta, unsigned int needs, bool exclusive);

private:
    const std::string level ();
    void tick ();
    void tick_color_base ();

    bool in_subsystem (edg_t e, const subsystem_t &s);
    bool stop_explore (edg_t e, const subsystem_t &s);
    bool prune (const jobmeta_t &meta, const bool exclusive,
                const std::string &s, vtx_t u,
                const std::vector<Resource> &resources);

    planner_t *subplan (vtx_t u, std::vector<uint64_t> &avail,
                        std::vector<const char *> &types);
    const std::vector<Resource> &next (const std::string &type,
                                       const std::vector<Resource> &resources);

    int accm_if (const subsystem_t &s, const std::string &type,
                 unsigned int cnt, std::map<std::string, int64_t> &acc);
    int accm_if (const subsystem_t &s, const std::string &type,
                 unsigned int cnt,
                 std::unordered_map<std::string, int64_t> &acc);

    // walk for resource match
    int explore (const jobmeta_t &meta, vtx_t u, const subsystem_t &s,
                 const std::vector<Resource> &resources, bool *exclusive,
                 bool *leaf, visit_t direction, scoring_api_t &to_parent);
    int aux_upv (const jobmeta_t &meta, vtx_t u, const subsystem_t &s,
                 const std::vector<Resource> &resources, bool *exclusive,
                 scoring_api_t &to_parent);
    int dom_exp (const jobmeta_t &meta, vtx_t u,
                 const std::vector<Resource> &resources, bool *exclusive,
                 bool *leaf, scoring_api_t &to_parent);
    int dom_dfv (const jobmeta_t &meta, vtx_t u,
                 const std::vector<Resource> &resources,
                 bool *exclusive, scoring_api_t &to_parent);

    // walk for resource schedule update and emit R
    int emit_edge (edg_t e);
    int emit_vertex (vtx_t u, unsigned int needs, bool exclusive);
    int updcore (vtx_t u, const subsystem_t &s, unsigned int needs,
                 bool exclusive, int n, const jobmeta_t &meta,
                 std::map<std::string, int64_t> &dfu,
                 std::map<std::string, int64_t> &to_parent);
    int upd_upv (vtx_t u, const subsystem_t &s, unsigned int needs,
                 bool exclusive, const jobmeta_t &meta,
                 std::map<std::string, int64_t> &to_parent);
    int upd_dfv (vtx_t u, unsigned int needs,
                 bool exclusive, const jobmeta_t &meta,
                 std::map<std::string, int64_t> &to_parent);

    // resolve and enforce hierarchical constraints
    int resolve (vtx_t root, std::vector<Resource> &resources,
                 scoring_api_t &dfu, bool exclusive, unsigned int *needs);
    int resolve (scoring_api_t &dfu, scoring_api_t &to_parent);
    int enforce (const subsystem_t &s, scoring_api_t &dfu);

    // private data
    color_t m_color;
    uint64_t m_best_k_cnt = 0;
    uint64_t m_color_base = 0;
    unsigned int m_trav_level = 0;
    std::map<subsystem_t, vtx_t> *m_roots = NULL;
    f_resource_graph_t *m_graph = NULL;
    dfu_match_cb_t *m_match = NULL;
};


/****************************************************************************
 *                                                                          *
 *                  DFU Traverser Implementation API Definitions            *
 *                                                                          *
 ****************************************************************************/

dfu_impl_t::dfu_impl_t ()
{

}

dfu_impl_t::dfu_impl_t (f_resource_graph_t *g, dfu_match_cb_t *m,
                        std::map<subsystem_t, vtx_t> *roots)
    : m_graph (g), m_match (m), m_roots (roots)
{

}

dfu_impl_t::dfu_impl_t (const dfu_impl_t &o)
{
    m_color = o.m_color;
    m_best_k_cnt = o.m_best_k_cnt;
    m_color_base = o.m_color_base;
    m_trav_level = o.m_trav_level;
    m_roots = o.m_roots;
    m_graph = o.m_graph;
    m_match = o.m_match;
}

dfu_impl_t &dfu_impl_t::operator= (const dfu_impl_t &o)
{
    m_color = o.m_color;
    m_best_k_cnt = o.m_best_k_cnt;
    m_color_base = o.m_color_base;
    m_trav_level = o.m_trav_level;
    m_roots = o.m_roots;
    m_graph = o.m_graph;
    m_match = o.m_match;
    return *this;
}

dfu_impl_t::~dfu_impl_t ()
{

}

const f_resource_graph_t *dfu_impl_t::get_graph () const
{
   return m_graph;
}

const std::map<subsystem_t, vtx_t> *dfu_impl_t::get_roots () const
{
    return m_roots;
}

const dfu_match_cb_t *dfu_impl_t::get_match_cb () const
{
    return m_match;
}

void dfu_impl_t::set_graph (f_resource_graph_t *g)
{
    m_graph = g;
}

void dfu_impl_t::set_roots (std::map<subsystem_t, vtx_t> *roots)
{
    m_roots = roots;
}

void dfu_impl_t::set_match_cb (dfu_match_cb_t *m)
{
    m_match = m;
}

int dfu_impl_t::prime (const subsystem_t &s, vtx_t u,
                       std::map<std::string, int64_t> &to_parent)
{
    int rc = 0;
    std::string type = (*m_graph)[u].type;
    std::map<std::string, int64_t> dfv;
    std::vector <const char *> types;
    std::vector<uint64_t> avail;

    (*m_graph)[u].idata.colors[s] = m_color.gray (m_color_base);
    accm_if (s, type, (*m_graph)[u].size, to_parent);
    graph_traits<f_resource_graph_t>::out_edge_iterator ei, ei_end;
    for (tie (ei, ei_end) = out_edges (u, *m_graph); ei != ei_end; ++ei) {
        if (!in_subsystem (*ei, s) || stop_explore (*ei, s))
            continue;
        if ((rc = prime (s, target (*ei, *m_graph), dfv)) != 0)
            goto done;
    }

    for (auto &agg : dfv) {
        accm_if (s, agg.first, agg.second, to_parent);
        types.push_back (strdup (agg.first.c_str ()));
        avail.push_back (agg.second);
    }

    if (!avail.empty () && !types.empty ())
        (*m_graph)[u].idata.subplans[s] = subplan (u, avail, types);

    (*m_graph)[u].idata.colors[s] = m_color.black (m_color_base);

done:
    if (!types.empty ())
        for (int i = 0; i < types.size (); ++i)
            free ((void *)types[i]);
    return rc;
}

int dfu_impl_t::prime (std::vector<Resource> &resources,
                       std::unordered_map<std::string, int64_t> &to_parent)
{
    int rc = 0;
    const subsystem_t &s = m_match->dom_subsystem ();
    for (auto &resource : resources) {
        accm_if (s, resource.type, resource.count.min, to_parent);
        if ((rc = prime (resource.with, resource.user_data)) != 0)
            goto done;
        for (auto &agg : resource.user_data)
            accm_if (s, agg.first, agg.second, to_parent);
    }
done:
    return rc;
}

int dfu_impl_t::select (Jobspec &j, vtx_t root, jobmeta_t &meta, bool exclusive,
                        unsigned int *needs)
{
    int rc = -1;
    scoring_api_t dfu;
    bool x_in = exclusive;
    const std::string &dom = m_match->dom_subsystem ();

    tick ();
    rc = dom_dfv (meta, root, j.resources, &x_in, dfu);
    if (rc == 0) {
        int64_t sc = dfu.overall_score ();
        unsigned int cn = dfu.avail ();
        dfu.add (dom, (*m_graph)[root].type, sc, cn, exclusive, true);
        rc = resolve (root, j.resources, dfu, exclusive, needs);
    }
    return rc;
}

int dfu_impl_t::update (vtx_t root, jobmeta_t &meta, unsigned int needs,
                        bool exclusive)
{
    int rc = -1;
    std::map<std::string, int64_t> dfu;

    tick_color_base ();
    rc = upd_dfv (root, needs, exclusive, meta, dfu);
    return (rc > 0)? 0 : -1;
}


/****************************************************************************
 *                                                                          *
 *         DFU Traverser Implementation Private API Definitions             *
 *                                                                          *
 ****************************************************************************/

const std::string dfu_impl_t::level ()
{
    int i;
    std::string prefix = "";
    for (i = 0; i < m_trav_level; ++i)
        prefix += "---";
    return prefix;
}

void dfu_impl_t::tick ()
{
    m_best_k_cnt++;
    m_color_base = m_color.reset (m_color_base);
}

void dfu_impl_t::tick_color_base ()
{
    m_color_base = m_color.reset (m_color_base);
}

bool dfu_impl_t::in_subsystem (edg_t e, const subsystem_t &s)
{
    return ((*m_graph)[e].idata.member_of.find (s)
                != (*m_graph)[e].idata.member_of.end ());
}

bool dfu_impl_t::stop_explore (edg_t e, const subsystem_t &s)
{
    vtx_t u = target (e, *m_graph);
    return ((*m_graph)[u].idata.colors[s] == m_color.gray (m_color_base)
            || (*m_graph)[u].idata.colors[s] == m_color.black (m_color_base));

}

bool dfu_impl_t::exclusivity (const std::vector<Resource> &res, vtx_t u)
{
    bool exclusive = false;
    for (auto &resource: res) {
        if (resource.type == (*m_graph)[u].type)
            exclusive = resource.exclusive;
    }
    return exclusive;
}

bool dfu_impl_t::prune (const jobmeta_t &meta, bool exclusive,
                        const std::string &s, vtx_t u,
                        const std::vector<Resource> &resources)
{
    bool rc = false;
    planner_t *p = NULL;
    int64_t avail;
    for (auto &resource : resources) {
        if ((*m_graph)[u].type != resource.type)
            continue;

        // prune by tag
        if (meta.allocate && exclusive
            && !(*m_graph)[u].schedule.tags.empty ()) {
            rc = true;
            break;
        }

        // prune by the planner
        p = (*m_graph)[u].schedule.plans;
        avail = planner_avail_resources_during (p, meta.at, meta.duration, 0);
        if (avail == 0) {
            rc = true;
            break;
        }

        // prune by the sub-planner
        if (resource.user_data.empty ())
            continue;
        std::vector<uint64_t> aggs;
        p = (*m_graph)[u].idata.subplans[s];
        count (p, resource.user_data, aggs);
        if (aggs.empty ())
            continue;
        size_t len = aggs.size ();
        if (planner_avail_during (p, meta.at, meta.duration, &(aggs[0]), len)) {
            rc = true;
            break;
        }
    }
    return rc;
}

planner_t *dfu_impl_t::subplan (vtx_t u, std::vector<uint64_t> &av,
                                std::vector<const char *> &tp)
{
    size_t len = av.size ();
    int64_t base_time = planner_base_time ((*m_graph)[u].schedule.plans);
    uint64_t duration = planner_duration ((*m_graph)[u].schedule.plans);
    return planner_new (base_time, duration, &av[0], &tp[0], len);
}

const std::vector<Resource> &dfu_impl_t::next (const std::string &type,
                                           const std::vector<Resource> &resources)
{
    for (auto &resource : resources)
        if (type == resource.type)
                return resource.with;
    return resources;
}

int dfu_impl_t::accm_if (const subsystem_t &s, const std::string &type,
                         unsigned int counts, std::map<std::string, int64_t> &acc)
{
   if (m_match->sdau_resource_types[s].find (type)
       != m_match->sdau_resource_types[s].end ()) {
       if (acc.find (type) == acc.end ())
           acc[type] = counts;
       else
           acc[type] += counts;
   }
   return 0;
}

int dfu_impl_t::accm_if (const subsystem_t &s, const std::string &type,
                         unsigned int counts,
                         std::unordered_map<std::string, int64_t> &acc)
{
   if (m_match->sdau_resource_types[s].find (type)
       != m_match->sdau_resource_types[s].end ()) {
       if (acc.find (type) == acc.end ())
           acc[type] = counts;
       else
           acc[type] += counts;
   }
   return 0;
}

int dfu_impl_t::explore (const jobmeta_t &meta, vtx_t u, const subsystem_t &s,
                         const std::vector<Resource> &resources, bool *exclusive,
                         bool *leaf, visit_t direction, scoring_api_t &dfu)
{
    int rc = -1;
    const subsystem_t &dom = m_match->dom_subsystem ();
    graph_traits<f_resource_graph_t>::out_edge_iterator ei, ei_end;
    for (tie (ei, ei_end) = out_edges (u, *m_graph); ei != ei_end; ++ei) {
        if (!in_subsystem (*ei, s) || stop_explore (*ei, s))
            continue;
        if (s == dom)
            *leaf = false;

        bool x_inout = *exclusive;
        vtx_t tgt = target (*ei, *m_graph);
        switch (direction) {
        case UPV:
            rc = aux_upv (meta, tgt, s, resources, &x_inout, dfu);
            break;
        case DFV:
        default:
            rc = dom_dfv (meta, tgt, resources, &x_inout, dfu);
            break;
        }

        if (rc == 0) {
            int64_t sc = dfu.overall_score ();
            unsigned int cn = dfu.avail ();
            dfu.add (s, (*m_graph)[tgt].type, sc, cn, x_inout, *ei);
        }
    }
    return rc;
}

int dfu_impl_t::aux_upv (const jobmeta_t &meta, vtx_t u, const subsystem_t &aux,
                         const std::vector<Resource> &resources, bool *exclusive,
                         scoring_api_t &to_parent)
{
    int rc = -1;
    scoring_api_t upv;
    bool x_in = *exclusive;
    planner_t *p = NULL;

    if (prune (meta, x_in, aux, u, resources))
        goto done;
    if ((rc = m_match->aux_discover_vtx (u, aux, resources, *m_graph)) != 0)
        goto done;

    (*m_graph)[u].idata.colors[aux] = m_color.gray (m_color_base);

    if (u != (*m_roots)[aux]) {
        graph_traits<f_resource_graph_t>::out_edge_iterator ei, ei_end;
        for (tie (ei, ei_end) = out_edges (u, *m_graph); ei != ei_end; ++ei) {
            if (!in_subsystem (*ei, aux) || stop_explore (*ei, aux))
                continue;
            bool x_inout = x_in;
            vtx_t tgt = target (*ei, *m_graph);
            rc = aux_upv (meta, tgt, aux, resources, &x_inout, upv);
            if (rc != 0)
                goto done;
        }
    }

    (*m_graph)[u].idata.colors[aux] = m_color.black (m_color_base);

    p = (*m_graph)[u].schedule.plans;
    if (planner_avail_resources_during (p, meta.at, meta.duration, 0) == 0)
        goto done;
    rc = m_match->aux_finish_vtx (u, aux, resources, *m_graph, upv);
    if (rc != 0)
        goto done;
    if ((rc = resolve (upv, to_parent)) != 0)
        goto done;

done:
    return rc;
}

int dfu_impl_t::dom_exp (const jobmeta_t &meta, vtx_t u,
                         const std::vector<Resource> &resources, bool *exclusive,
                         bool *leaf, scoring_api_t &dfu)
{
    int rc = -1;
    const subsystem_t &dom = m_match->dom_subsystem ();
    for (auto &s : m_match->subsystems ()) {
        if (s == dom)
            rc = explore (meta, u, s, resources, exclusive, leaf, DFV, dfu);
        else
            rc = explore (meta, u, s, resources, exclusive, leaf, UPV, dfu);
    }
    return rc;
}

int dfu_impl_t::dom_dfv (const jobmeta_t &meta, vtx_t u,
                         const std::vector<Resource> &resources, bool *exclusive,
                         scoring_api_t &to_parent)
{
    int rc = -1;
    uint64_t avail = 0;
    bool leaf = true;
    bool x_in = *exclusive || exclusivity (resources, u);
    bool x_inout = x_in;
    scoring_api_t dfu;
    planner_t *p = NULL;
    const std::string &dom = m_match->dom_subsystem ();
    const std::vector<Resource> &nx = next ((*m_graph)[u].type, resources);
    graph_traits<f_resource_graph_t>::out_edge_iterator ei, ei_end;

    if (prune (meta, x_in, dom, u, resources))
        goto done;
    rc = m_match->dom_discover_vtx (u, dom, resources, *m_graph);
    if (rc != 0)
        goto done;

    (*m_graph)[u].idata.colors[dom] = m_color.gray (m_color_base);
    rc = dom_exp (meta, u, nx, &x_inout, &leaf, dfu);

    *exclusive = x_in; // Leaf here for debugging
    (*m_graph)[u].idata.colors[dom] = m_color.black (m_color_base);

    p = (*m_graph)[u].schedule.plans;
    avail = planner_avail_resources_during (p, meta.at, meta.duration, 0);
    if (avail == 0)
        goto done;
    rc = m_match->dom_finish_vtx (u, dom, resources, *m_graph, dfu);
    if (rc != 0)
        goto done;
    if ((rc = resolve (dfu, to_parent)) != 0)
        goto done;
    to_parent.set_avail (avail);
    to_parent.set_overall_score (dfu.overall_score ());
done:
    return rc;
}

int dfu_impl_t::resolve (vtx_t root, std::vector<Resource> &resources,
                         scoring_api_t &dfu, bool exclusive, unsigned int *needs)
{
    int rc = 0;
    unsigned int qc;
    unsigned int count;
    const subsystem_t &dom = m_match->dom_subsystem ();

    rc = m_match->dom_finish_graph (dom, resources, *m_graph, dfu);
    if ( rc != 0)
        goto done;

    for (auto &resource : resources) {
        if (resource.type == (*m_graph)[root].type) {
            qc = dfu.avail ();
            count = m_match->select_count (resource, qc);
            if (count == 0) {
                rc = -1;
                goto done;
            }
            *needs = count;
        }
    }

    // resolve remaining unresolved resource types
    for (auto &subsystem : m_match->subsystems ()) {
        std::vector<std::string> types;
        dfu.resrc_types (subsystem, types);
        for (auto &type : types) {
            if (dfu.qualified_count (subsystem, type) == 0) {
                rc = -1;
                goto done;
            } else if (!dfu.best_k (subsystem, type)) {
                dfu.choose_accum_all (subsystem, type);
            }
        }
    }

    for (auto subsystem : m_match->subsystems ())
        enforce (subsystem, dfu);

done:
    return rc;
}

int dfu_impl_t::resolve (scoring_api_t &dfu, scoring_api_t &to_parent)
{
    int rc = 0;
    if (dfu.overall_score () > 0) {
        if (dfu.hier_constrain_now ()) {
            for (auto subsystem : m_match->subsystems ())
                enforce (subsystem, dfu);
        }
        else {
            to_parent.merge (dfu);
        }
    }
    return rc;
}

int dfu_impl_t::enforce (const subsystem_t &ss, scoring_api_t &dfu)
{
    int rc = 0;
    try {
        std::vector<std::string> resource_types;
        dfu.resrc_types (ss, resource_types);
        for (auto &t : resource_types) {
            int best_i = dfu.best_i (ss, t);
            for (int i = 0; i < best_i; i++) {
                if (dfu.at (ss, t, i).root)
                    continue;
                edg_t e = dfu.at (ss, t, i).edge;
                (*m_graph)[e].idata.needs = dfu.at (ss, t, i).needs;
                (*m_graph)[e].idata.best_k_cnt = m_best_k_cnt;
                (*m_graph)[e].idata.exclusive = dfu.at (ss, t, i).exclusive;
            }
        }
    } catch (const std::out_of_range &exception) {
        errno = ERANGE;
        rc = -1;
    }
    return rc;
}

int dfu_impl_t::count (planner_t *plan,
                       const std::map<std::string, int64_t> &lookup,
                       std::vector<uint64_t> &agg)
{
    int rc = 0;
    try {
        size_t len = planner_resources_len (plan);
        const char **resource_types = planner_resource_types (plan);
        for (int i = 0; i < len; ++i) {
            if (lookup.find (resource_types[i]) != lookup.end ())
                agg.push_back ((uint64_t)lookup.at (resource_types[i]));
            else
                agg.push_back (0);
        }
    } catch (const std::out_of_range &exception) {
        errno = ERANGE;
        rc = -1;
    }
    return 0;
}

int dfu_impl_t::count (planner_t *plan,
                       const std::unordered_map<std::string, int64_t> &lookup,
                       std::vector<uint64_t> &agg)
{
    int rc = 0;
    try {
        size_t len = planner_resources_len (plan);
        const char **resource_types = planner_resource_types (plan);
        for (int i = 0; i < len; ++i) {
            if (lookup.find (resource_types[i]) != lookup.end ())
                agg.push_back (lookup.at (resource_types[i]));
            else
                agg.push_back (0);
        }
    } catch (const std::out_of_range &exception) {
        errno = ERANGE;
        rc = -1;
    }
    return 0;
}

int dfu_impl_t::emit_edge (edg_t e)
{
    return 0; // NYI
}

int dfu_impl_t::emit_vertex (vtx_t u, unsigned int needs, bool exclusive)
{
    std::string mode = (exclusive)? "X" : "S";
    std::cout << "      " << level () << (*m_graph)[u].name << "["
              << needs << ":" << mode  << "]" << std::endl;
    return 0;
}

int dfu_impl_t::updcore (vtx_t u, const subsystem_t &s, unsigned int needs,
                         bool exclusive, int n, const jobmeta_t &meta,
                         std::map<std::string, int64_t> &dfu,
                         std::map<std::string, int64_t> &to_parent)
{
    int64_t span = -1;

    if (exclusive) {
        const uint64_t u64needs = (const uint64_t)needs;
        n++;
        planner_t *plans = (*m_graph)[u].schedule.plans;
        span = planner_add_span (plans, meta.at, meta.duration, &u64needs, 1);
        accm_if (s, (*m_graph)[u].type, needs, to_parent);
    }

    if (n > 0) {
        planner_t *subplan = (*m_graph)[u].idata.subplans[s];
        if (meta.allocate)
            (*m_graph)[u].schedule.tags[meta.jobid] = meta.jobid;
        if (exclusive) {
            if (meta.allocate)
                (*m_graph)[u].schedule.allocations[meta.jobid] = span;
            else
                (*m_graph)[u].schedule.reservations[meta.jobid] = span;
        }
        if (subplan && !dfu.empty ()) {
            std::vector<uint64_t> agg;
            count (subplan, dfu, agg);
            span = planner_add_span (subplan, meta.at, meta.duration,
                                     &(agg[0]), agg.size ());
            (*m_graph)[u].idata.job2span[meta.jobid] = span;
        }
        for (auto &kv : dfu)
            accm_if (s, kv.first, kv.second, to_parent);

        emit_vertex (u, needs, exclusive);
    }
    m_trav_level--;
    return n;
}

int dfu_impl_t::upd_upv (vtx_t u, const subsystem_t &s, unsigned int needs,
                         bool exclusive, const jobmeta_t &meta,
                         std::map<std::string, int64_t> &to_parent)
{
    return 0; // NYI
}

int dfu_impl_t::upd_dfv (vtx_t u, unsigned int needs, bool exclusive,
                         const jobmeta_t &meta,
                         std::map<std::string, int64_t> &to_parent)
{
    int n_plans = 0;
    std::map<std::string, int64_t> dfu;
    const std::string &dom = m_match->dom_subsystem ();
    graph_traits<f_resource_graph_t>::out_edge_iterator ei, ei_end;
    m_trav_level++;

    for (auto &s : m_match->subsystems ()) {
        for (tie (ei, ei_end) = out_edges (u, *m_graph); ei != ei_end; ++ei) {
            if (!in_subsystem (*ei, s) || stop_explore (*ei, s))
                continue;
            if ((*m_graph)[*ei].idata.best_k_cnt != m_best_k_cnt)
                continue;
            bool x = (*m_graph)[*ei].idata.exclusive;
            unsigned int needs = (*m_graph)[*ei].idata.needs;
            vtx_t tgt = target (*ei, *m_graph);
            n_plans += (s == dom)? upd_dfv (tgt, needs, x, meta, dfu)
                                 : upd_upv (tgt, s, needs, x, meta, dfu);
            if (n_plans > 0)
                emit_edge (*ei);
        }
    }

    (*m_graph)[u].idata.colors[dom] = m_color.black (m_color_base);

    return updcore (u, dom, needs, exclusive, n_plans, meta, dfu, to_parent);
}

} // namespace detail

/*! Depth-First-and-Up traverser. Perform depth-first visit on the dominant
 *  subsystem and upwalk on each and all of the auxiliary subsystems selected
 *  by the matcher whose type. The matcher type is passed in as a template
 *  argument. Call back match callback methods at various visit events.
 */
class dfu_traverser_t : protected detail::dfu_impl_t
{
public:
    dfu_traverser_t ();
    dfu_traverser_t (f_resource_graph_t *g,
                     dfu_match_cb_t *m,
                     std::map<subsystem_t, vtx_t> *roots);
    dfu_traverser_t (const dfu_traverser_t &o);
    dfu_traverser_t &operator= (const dfu_traverser_t &o);
    ~dfu_traverser_t ();

    const f_resource_graph_t *get_graph () const;
    const std::map<subsystem_t, vtx_t> *get_roots () const;
    const dfu_match_cb_t *get_match_cb () const;

    void set_graph (f_resource_graph_t *g);
    void set_roots (std::map<subsystem_t, vtx_t> *roots);
    void set_match_cb (dfu_match_cb_t *m);

    int initialize ();
    int initialize (f_resource_graph_t *g, std::map<subsystem_t, vtx_t> *roots,
                    dfu_match_cb_t *m);
    int run (Jobspec &jspec, match_op_t op, int64_t jobid, int64_t *at);

};


/****************************************************************************
 *                                                                          *
 *                    DFU Traverser Public API Definitions                  *
 *                                                                          *
 ****************************************************************************/

dfu_traverser_t::dfu_traverser_t ()
{

}

dfu_traverser_t::dfu_traverser_t (f_resource_graph_t *g, dfu_match_cb_t *m,
                                  std::map<subsystem_t, vtx_t> *roots)
    : detail::dfu_impl_t (g, m, roots)
{

}

dfu_traverser_t::dfu_traverser_t (const dfu_traverser_t &o)
    : detail::dfu_impl_t (o)
{

}

dfu_traverser_t &dfu_traverser_t::operator= (const dfu_traverser_t &o)
{
    detail::dfu_impl_t::operator= (o);
    return *this;
}

dfu_traverser_t::~dfu_traverser_t ()
{

}

const f_resource_graph_t *dfu_traverser_t::get_graph () const
{
   return detail::dfu_impl_t::get_graph ();
}

const std::map<subsystem_t, vtx_t> *dfu_traverser_t::get_roots () const
{
    return detail::dfu_impl_t::get_roots ();
}

const dfu_match_cb_t *dfu_traverser_t::get_match_cb () const
{
    return detail::dfu_impl_t::get_match_cb ();
}

void dfu_traverser_t::set_graph (f_resource_graph_t *g)
{
    detail::dfu_impl_t::set_graph (g);
}

void dfu_traverser_t::set_roots (std::map<subsystem_t, vtx_t> *roots)
{
    detail::dfu_impl_t::set_roots (roots);
}

void dfu_traverser_t::set_match_cb (dfu_match_cb_t *m)
{
    detail::dfu_impl_t::set_match_cb (m);
}

int dfu_traverser_t::initialize ()
{
    if (!get_graph () || !get_roots () || !get_match_cb ()) {
        errno = EINVAL;
        return -1;
    }

    int rc = 0;
    try {
        vtx_t root;
        for (auto &subsystem : get_match_cb ()->subsystems ()) {
            std::map<std::string, int64_t> from_dfv;
            root = get_roots ()->at (subsystem);
            rc += detail::dfu_impl_t::prime (subsystem, root,
                                                           from_dfv);
        }
    } catch (const std::out_of_range &exception) {
        errno = ERANGE;
        rc = -1;
    }
    return rc;
}

int dfu_traverser_t::initialize (f_resource_graph_t *g,
                                 std::map<subsystem_t, vtx_t> *roots,
                                 dfu_match_cb_t *m)
{
    set_graph (g);
    set_roots (roots);
    set_match_cb (m);
    return initialize ();
}

int dfu_traverser_t::run (Jobspec &j, match_op_t op, int64_t jobid, int64_t *at)
{
    const subsystem_t &dom = get_match_cb ()->dom_subsystem ();
    if (!get_graph () || !get_roots ()
        || get_roots ()->find (dom) == get_roots ()->end ()
        || !get_match_cb () || j.resources.empty ()) {
        errno = EINVAL;
        return -1;
    }

    int rc = -1;
    detail::jobmeta_t meta;
    unsigned int needs = 0;
    vtx_t root = get_roots ()->at (dom);
    bool ex = detail::dfu_impl_t::exclusivity (j.resources, root);
    std::unordered_map<std::string, int64_t> dfv;
    if (detail::dfu_impl_t::prime (j.resources, dfv))
        return -1;

    /* Allocate */
    meta.build (j, true, jobid, *at);
    rc = detail::dfu_impl_t::select (j, root, meta, ex, &needs);
    if ((rc != 0) && (op == MATCH_ALLOCATE_ORELSE_RESERVE)) {
        /* Or else reserve */
        meta.allocate = false;
        int64_t t = meta.at + 1;
        std::vector<uint64_t> agg;
        planner_t *p = (*get_graph ())[root].idata.subplans.at (dom);
        size_t len = planner_resources_len (p);
        detail::dfu_impl_t::count (p, dfv, agg);
        for (t = planner_avail_time_first (p, t, meta.duration, &agg[0], len);
             t != -1 && rc != 0; t = planner_avail_time_next (p)) {
            meta.at = t;
            rc = detail::dfu_impl_t::select (j, root, meta, ex, &needs);
        }
    }

    if (rc == 0) {
        *at = meta.at;
        rc = detail::dfu_impl_t::update (root, meta, needs, ex);
    }

    return rc;
}

} // namespace resource_model

#endif // DFU_TRAVERSE_HPP

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
