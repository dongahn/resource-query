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

#include "dfu_traverse_impl.hpp"

using namespace std;
using namespace Flux::Jobspec;
using namespace Flux::resource_model;
using namespace Flux::resource_model::detail;

/****************************************************************************
 *                                                                          *
 *         DFU Traverser Implementation Private API Definitions             *
 *                                                                          *
 ****************************************************************************/

const std::string dfu_impl_t::level () const
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

bool dfu_impl_t::in_subsystem (edg_t e, const subsystem_t &s) const
{
    return ((*m_graph)[e].idata.member_of.find (s)
                != (*m_graph)[e].idata.member_of.end ());
}

bool dfu_impl_t::stop_explore (edg_t e, const subsystem_t &s) const
{
    vtx_t u = target (e, *m_graph);
    return ((*m_graph)[u].idata.colors[s] == m_color.gray (m_color_base)
            || (*m_graph)[u].idata.colors[s] == m_color.black (m_color_base));

}

bool dfu_impl_t::exclusivity (const std::vector<Jobspec::Resource> &resources,
                              vtx_t u)
{
    bool exclusive = false;
    for (auto &resource: resources) {
        if (resource.type == (*m_graph)[u].type)
            if (resource.exclusive == Jobspec::tristate_t::TRUE)
                exclusive = true;
    }
    return exclusive;
}

bool dfu_impl_t::prune (const jobmeta_t &meta, bool exclusive,
                        const std::string &s, vtx_t u,
                        const std::vector<Jobspec::Resource> &resources)
{
    bool rc = false;
    planner_t *p = NULL;
    int64_t avail;

    // Prune by the visiting resource vertex's availability
    // if rack has been allocated exclusivley, no reason to descend
    p = (*m_graph)[u].schedule.plans;
    avail = planner_avail_resources_during (p, meta.at, meta.duration, 0);
    if (avail == 0) {
        rc = true;
        goto done;
    }

    // Prune by tag or subtree plan if on of the resource request
    // matches with the visiting vertex
    for (auto &resource : resources) {
        if ((*m_graph)[u].type != resource.type)
            continue;

        // prune by tag
        if (meta.allocate && exclusive
            && !(*m_graph)[u].schedule.tags.empty ()) {
            rc = true;
            break;
        }

        // prune by the subtree planner
        if (resource.user_data.empty ())
            continue;
        vector<uint64_t> aggs;
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
done:
    return rc;
}

planner_t *dfu_impl_t::subtree_plan (vtx_t u, vector<uint64_t> &av,
                                     vector<const char *> &tp)
{
    size_t len = av.size ();
    int64_t base_time = planner_base_time ((*m_graph)[u].schedule.plans);
    uint64_t duration = planner_duration ((*m_graph)[u].schedule.plans);
    return planner_new (base_time, duration, &av[0], &tp[0], len);
}

void dfu_impl_t::match (vtx_t u, const vector<Resource> &resources,
                       const Resource **slot_resource,
                       const Resource **match_resource)
{
    for (auto &resource : resources) {
        if ((*m_graph)[u].type == resource.type) {
            *match_resource = &resource;
            if (!resource.with.empty ()) {
                for (auto &c_resource : resource.with)
                    if (c_resource.type == "slot")
                        *slot_resource = &c_resource;
            }
            break; // limitations: jobspec must not have same type at same level
        } else if (resource.type == "slot") {
            *slot_resource = &resource;
            break;
        }
    }
}

bool dfu_impl_t::slot_match (vtx_t u, const Resource *slot_resources)
{
    bool slot_match = true;
    graph_traits<f_resource_graph_t>::out_edge_iterator ei, ei_end;
    if (slot_resources) {
        for (auto &c_resource : (*slot_resources).with) {
            for (tie (ei, ei_end) = out_edges (u, *m_graph); ei != ei_end; ++ei) {
                vtx_t tgt = target (*ei, *m_graph);
                if ((*m_graph)[tgt].type == c_resource.type)
                    break; // found the destination resource type of the slot
            }
            if (ei == ei_end) {
                slot_match = false;
                break;
            }
        }
    } else {
        slot_match = false;
    }
    return slot_match;
}

const vector<Resource> &dfu_impl_t::test (vtx_t u,
                                          const vector<Resource> &resources,
                                          match_kind_t *spec)
{
    bool slot = true;
    const vector<Resource> *ret = &resources;
    const Resource *slot_resources = NULL;
    const Resource *match_resources = NULL;
    match (u, resources, &slot_resources, &match_resources);
    if ( (slot = slot_match (u, slot_resources))) {
        *spec = match_kind_t::SLOT_MATCH;
        ret = &(slot_resources->with);
    } else if (match_resources) {
        *spec = match_kind_t::RESOURCE_MATCH;
        ret = &(match_resources->with);
    } else {
        *spec = match_kind_t::NO_MATCH;
    }
    return *ret;
}

int dfu_impl_t::accum_if (const subsystem_t &subsystem, const string &type,
                          unsigned int counts, map<string, int64_t> &accum)
{
    int rc = -1;
    if (m_match->sdau_resource_types[subsystem].find (type)
        != m_match->sdau_resource_types[subsystem].end ()) {
        if (accum.find (type) == accum.end ())
            accum[type] = counts;
        else
            accum[type] += counts;
        rc = 0;
    }
    return rc;
}

int dfu_impl_t::accum_if (const subsystem_t &subsystem, const string &type,
                          unsigned int counts,
                          std::unordered_map<string, int64_t> &accum)
{
    int rc = -1;
    if (m_match->sdau_resource_types[subsystem].find (type)
        != m_match->sdau_resource_types[subsystem].end ()) {
        if (accum.find (type) == accum.end ())
            accum[type] = counts;
        else
            accum[type] += counts;
        rc = 0;
    }
    return rc;
}

int dfu_impl_t::prime_exp (const subsystem_t &subsystem, vtx_t u,
                           map<string, int64_t> &dfv)
{
    int rc = 0;
    graph_traits<f_resource_graph_t>::out_edge_iterator ei, ei_end;
    for (tie (ei, ei_end) = out_edges (u, *m_graph); ei != ei_end; ++ei) {
        if (!in_subsystem (*ei, subsystem) || stop_explore (*ei, subsystem))
            continue;
        if ((rc = prime (subsystem, target (*ei, *m_graph), dfv)) != 0)
            break;
    }
    return rc;
}

int dfu_impl_t::explore (const jobmeta_t &meta, vtx_t u,
                         const subsystem_t &subsystem,
                         const vector<Resource> &resources, bool *excl,
                         bool *leaf, visit_t direction, scoring_api_t &dfu)
{
    int rc = -1;
    int rc2 = -1;
    const subsystem_t &dom = m_match->dom_subsystem ();
    graph_traits<f_resource_graph_t>::out_edge_iterator ei, ei_end;
    for (tie (ei, ei_end) = out_edges (u, *m_graph); ei != ei_end; ++ei) {
        if (!in_subsystem (*ei, subsystem) || stop_explore (*ei, subsystem))
            continue;
        if (subsystem == dom)
            *leaf = false;

        bool x_inout = *excl;
        vtx_t tgt = target (*ei, *m_graph);
        switch (direction) {
        case visit_t::UPV:
            rc = aux_upv (meta, tgt, subsystem, resources, &x_inout, dfu);
            break;
        case visit_t::DFV:
        default:
            rc = dom_dfv (meta, tgt, resources, &x_inout, dfu);
            break;
        }

        if (rc == 0) {
            int64_t score = dfu.overall_score ();
            unsigned int count = dfu.avail ();
            eval_edg_t ev_edg (count, count, x_inout, *ei);
            eval_egroup_t singleton;
            singleton.score = score;
            singleton.count = count;
            singleton.exclusive = x_inout;
            singleton.edges.push_back (ev_edg);
            dfu.add (subsystem, (*m_graph)[tgt].type, singleton);
            rc2 = 0;
        }
    }
    return rc2;
}

int dfu_impl_t::aux_upv (const jobmeta_t &meta, vtx_t u, const subsystem_t &aux,
                         const vector<Resource> &resources, bool *excl,
                         scoring_api_t &to_parent)
{
    int rc = -1;
    scoring_api_t upv;
    bool x_in = *excl;
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
            if ((rc = aux_upv (meta, tgt, aux, resources, &x_inout, upv)) != 0)
                goto done;
        }
    }
    (*m_graph)[u].idata.colors[aux] = m_color.black (m_color_base);

    p = (*m_graph)[u].schedule.plans;
    if (planner_avail_resources_during (p, meta.at, meta.duration, 0) == 0)
        goto done;
    if ((rc = m_match->aux_finish_vtx (u, aux, resources, *m_graph, upv)) != 0)
        goto done;
    if ((rc = resolve (upv, to_parent)) != 0)
        goto done;
done:
    return rc;
}

int dfu_impl_t::dom_exp (const jobmeta_t &meta, vtx_t u,
                         const vector<Resource> &resources,
                         bool *excl, bool *leaf, scoring_api_t &dfu)
{
    int rc = -1;
    const subsystem_t &dom = m_match->dom_subsystem ();
    for (auto &s : m_match->subsystems ()) {
        if (s == dom)
            rc = explore (meta, u, s, resources, excl, leaf, visit_t::DFV, dfu);
        else
            rc = explore (meta, u, s, resources, excl, leaf, visit_t::UPV, dfu);
    }
    return rc;
}

int dfu_impl_t::dom_slot (const jobmeta_t &meta, vtx_t u,
                          const vector<Resource> &slot_shape,
                          bool *excl, bool *leaf, scoring_api_t &dfu)
{
    int rc;
    bool x_inout = true;
    scoring_api_t dfu_slot;
    unsigned int qual_num_slots = 0;
    const subsystem_t &dom = m_match->dom_subsystem ();
    rc = explore (meta, u, dom, slot_shape, &x_inout, leaf, visit_t::DFV, dfu_slot);
    if (rc != 0)
        goto done;
    if ((rc = m_match->dom_finish_slot (dom, dfu_slot)) != 0)
        goto done;

    qual_num_slots = UINT_MAX;
    // TODO: count max/min
    for (auto &slot_elem : slot_shape) {
        unsigned int qc = dfu_slot.qualified_count (dom, slot_elem.type);
        unsigned int fit = (qc / slot_elem.count.max);
        qual_num_slots = (qual_num_slots > fit)? fit : qual_num_slots;
        //unsigned int count = select_count (slot_elem, qc);
    }

    for (int i = 0; i < qual_num_slots; ++i) {
        eval_egroup_t edg_group;
        int score = MATCH_MET;
        for (auto &slot_elem : slot_shape) {
            int j = 0;
            if (i == 0)
                dfu_slot.rewind_iter_cur (dom, slot_elem.type);
            while (j < slot_elem.count.max) {
                auto egroup_i = dfu_slot.iter_cur (dom, slot_elem.type);
                eval_edg_t ev_edg ((*egroup_i).edges[0].count,
                                   (*egroup_i).edges[0].count, 1,
                                   (*egroup_i).edges[0].edge);
                score += (*egroup_i).score;
                edg_group.edges.push_back (ev_edg);
                j += (*egroup_i).edges[0].count;
                dfu_slot.incr_iter_cur (dom, slot_elem.type);
            }
        }
        edg_group.score = score;
        edg_group.count = 1;
        edg_group.exclusive = 1;
        dfu.add (dom, string ("slot"), edg_group);
    }

done:
    return (qual_num_slots)? 0 : -1;
}

int dfu_impl_t::dom_dfv (const jobmeta_t &meta, vtx_t u,
                         const vector<Resource> &resources, bool *excl,
                         scoring_api_t &to_parent)
{
    int rc = -1;
    match_kind_t sm;
    uint64_t avail = 0;
    bool leaf = true;
    bool x_in = *excl || exclusivity (resources, u);
    bool x_inout = x_in;
    scoring_api_t dfu;
    planner_t *p = NULL;
    const string &dom = m_match->dom_subsystem ();
    graph_traits<f_resource_graph_t>::out_edge_iterator ei, ei_end;

    const vector<Resource> &next = test (u, resources, &sm);
    if (prune (meta, x_in, dom, u, resources))
        goto done;
    if ((rc = m_match->dom_discover_vtx (u, dom, resources, *m_graph)) != 0)
        goto done;

    // BEGIN: coloring and recursion
    (*m_graph)[u].idata.colors[dom] = m_color.gray (m_color_base);
    if (sm == match_kind_t::SLOT_MATCH)
        rc = dom_slot (meta, u, next, &x_inout, &leaf, dfu);
    else
        rc = dom_exp (meta, u, next, &x_inout, &leaf, dfu);
    *excl = x_in;
    (*m_graph)[u].idata.colors[dom] = m_color.black (m_color_base);
    // END: coloring and recursion

    p = (*m_graph)[u].schedule.plans;
    avail = planner_avail_resources_during (p, meta.at, meta.duration, 0);
    if (avail == 0)
        goto done;
    if (m_match->dom_finish_vtx (u, dom, resources, *m_graph, dfu) != 0)
        goto done;
    if ((rc = resolve (dfu, to_parent)) != 0)
        goto done;
    to_parent.set_avail (avail);
    to_parent.set_overall_score (dfu.overall_score ());
done:
    return rc;
}

int dfu_impl_t::resolve (vtx_t root, vector<Resource> &resources,
                         scoring_api_t &dfu, bool excl, unsigned int *needs)
{
    int rc = 0;
    unsigned int qc;
    unsigned int count;
    const subsystem_t &dom = m_match->dom_subsystem ();
    if ((rc = m_match->dom_finish_graph (dom, resources, *m_graph, dfu)) != 0)
        goto done;

    for (auto &resource : resources) {
        if (resource.type == (*m_graph)[root].type) {
            qc = dfu.avail ();
            if ((count = m_match->select_count (resource, qc)) == 0) {
                rc = -1;
                goto done;
            }
            *needs = count;
        }
    }
    // resolve remaining unconstrained resource types
    for (auto &subsystem : m_match->subsystems ()) {
        vector<string> types;
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

int dfu_impl_t::enforce (const subsystem_t &subsystem, scoring_api_t &dfu)
{
    int rc = 0;
    try {
        vector<string> resource_types;
        dfu.resrc_types (subsystem, resource_types);
        for (auto &t : resource_types) {
            int best_i = dfu.best_i (subsystem, t);
            for (int i = 0; i < best_i; i++) {
                if (dfu.at (subsystem, t, i).root)
                    continue;
                const eval_egroup_t &egroup = dfu.at (subsystem, t, i);
                for (auto &e : egroup.edges) {
                    (*m_graph)[e.edge].idata.needs = e.needs;
                    (*m_graph)[e.edge].idata.best_k_cnt = m_best_k_cnt;
                    (*m_graph)[e.edge].idata.exclusive = e.exclusive;
                }
            }
        }
    } catch (const out_of_range &exception) {
        errno = ERANGE;
        rc = -1;
    }
    return rc;
}

int dfu_impl_t::emit_edge (edg_t e)
{
    // We will ultimately need to emit edge info for nested instance
    // with complex scheduler, pending a discussion on R.
    return 0;
}

int dfu_impl_t::emit_vertex (vtx_t u, unsigned int needs, bool exclusive)
{
    string mode = (exclusive)? "x" : "s";
    cout << "      " << level () << (*m_graph)[u].name << "["
         << needs << ":" << mode  << "]" << endl;
    return 0;
}

int dfu_impl_t::updcore (vtx_t u, const subsystem_t &s, unsigned int needs,
                         bool excl, int n, const jobmeta_t &meta,
                         map<string, int64_t> &dfu,
                         map<string, int64_t> &to_parent)
{
    int64_t span = -1;
    if (excl) {
        // TODO: Please explain
        const uint64_t u64needs = (const uint64_t)needs;
        n++;
        planner_t *plans = (*m_graph)[u].schedule.plans;
        span = planner_add_span (plans, meta.at, meta.duration, &u64needs, 1);
        accum_if (s, (*m_graph)[u].type, needs, to_parent);
    }

    if (n > 0) {
        planner_t *subtree_plan = (*m_graph)[u].idata.subplans[s];
        if (meta.allocate)
            (*m_graph)[u].schedule.tags[meta.jobid] = meta.jobid;
        if (excl) {
            if (meta.allocate)
                (*m_graph)[u].schedule.allocations[meta.jobid] = span;
            else
                (*m_graph)[u].schedule.reservations[meta.jobid] = span;
        }
        if (subtree_plan && !dfu.empty ()) {
            vector<uint64_t> aggregate;
            count (subtree_plan, dfu, aggregate);
            span = planner_add_span (subtree_plan, meta.at, meta.duration,
                                     &(aggregate[0]), aggregate.size ());
            (*m_graph)[u].idata.job2span[meta.jobid] = span;
        }
        for (auto &kv : dfu)
            accum_if (s, kv.first, kv.second, to_parent);
        emit_vertex (u, needs, excl);
    }
    m_trav_level--;
    return n;
}

int dfu_impl_t::upd_upv (vtx_t u, const subsystem_t &subsystem,
                         unsigned int needs, bool excl, const jobmeta_t &meta,
                         map<string, int64_t> &to_parent)
{
    //TODO: update resources on the UPV direction
    return 0;
}

int dfu_impl_t::upd_dfv (vtx_t u, unsigned int needs, bool excl,
                         const jobmeta_t &meta,
                         map<string, int64_t> &to_parent)
{
    int n_plans = 0;
    map<string, int64_t> dfu;
    const string &dom = m_match->dom_subsystem ();
    graph_traits<f_resource_graph_t>::out_edge_iterator ei, ei_end;
    m_trav_level++;

    for (auto &subsystem : m_match->subsystems ()) {
        for (tie (ei, ei_end) = out_edges (u, *m_graph); ei != ei_end; ++ei) {
            if (!in_subsystem (*ei, subsystem) || stop_explore (*ei, subsystem))
                continue;
            if ((*m_graph)[*ei].idata.best_k_cnt != m_best_k_cnt)
                continue;

            bool x = (*m_graph)[*ei].idata.exclusive;
            unsigned int needs = (*m_graph)[*ei].idata.needs;
            vtx_t tgt = target (*ei, *m_graph);
            if (subsystem == dom)
                n_plans += upd_dfv (tgt, needs, x, meta, dfu);
            else
                n_plans += upd_upv (tgt, subsystem, needs, x, meta, dfu);

            if (n_plans > 0)
                emit_edge (*ei);
        }
    }
    (*m_graph)[u].idata.colors[dom] = m_color.black (m_color_base);

    return updcore (u, dom, needs, excl, n_plans, meta, dfu, to_parent);
}


/****************************************************************************
 *                                                                          *
 *           DFU Traverser Implementation Public API Definitions            *
 *                                                                          *
 ****************************************************************************/

dfu_impl_t::dfu_impl_t ()
{

}

dfu_impl_t::dfu_impl_t (f_resource_graph_t *g, dfu_match_cb_t *m,
                        map<subsystem_t, vtx_t> *roots)
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

const map<subsystem_t, vtx_t> *dfu_impl_t::get_roots () const
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

void dfu_impl_t::set_roots (map<subsystem_t, vtx_t> *roots)
{
    m_roots = roots;
}

void dfu_impl_t::set_match_cb (dfu_match_cb_t *m)
{
    m_match = m;
}

int dfu_impl_t::prime (const subsystem_t &s, vtx_t u,
                       map<string, int64_t> &to_parent)
{
    int rc = -1;
    vector<uint64_t> avail;
    vector<const char *> types;
    map<string, int64_t> dfv;
    string type = (*m_graph)[u].type;

    (*m_graph)[u].idata.colors[s] = m_color.gray (m_color_base);
    accum_if (s, type, (*m_graph)[u].size, to_parent);
    if (prime_exp (s, u, dfv) != 0)
        goto done;

    for (auto &aggregate : dfv) {
        accum_if (s, aggregate.first, aggregate.second, to_parent);
        types.push_back (strdup (aggregate.first.c_str ()));
        avail.push_back (aggregate.second);
    }
    if (!avail.empty () && !types.empty ()) {
        planner_t *p = NULL;
        if (avail.size () > PLANNER_NUM_TYPES) {
            errno = ERANGE;
            goto done;
        } else if (!(p = subtree_plan (u, avail, types)) )
            goto done;
        (*m_graph)[u].idata.subplans[s] = p;
    }
    rc = 0;
done:
    (*m_graph)[u].idata.colors[s] = m_color.black (m_color_base);
    if (!types.empty ())
        for (int i = 0; i < types.size (); ++i)
            free ((void *)types[i]);
    return rc;
}

void dfu_impl_t::prime (vector<Resource> &resources,
                        std::unordered_map<string, int64_t> &to_parent)
{
    const subsystem_t &subsystem = m_match->dom_subsystem ();
    for (auto &resource : resources) {
        // Use minimum requirement because you don't want to prune search
        // as far as a subtree satisfies the minimum requirement
        accum_if (subsystem, resource.type, resource.count.min, to_parent);
        prime (resource.with, resource.user_data);
        for (auto &aggregate : resource.user_data)
            accum_if (subsystem, aggregate.first, aggregate.second, to_parent);
    }
}

int dfu_impl_t::select (Jobspec::Jobspec &j, vtx_t root, jobmeta_t &meta,
                        bool excl, unsigned int *needs)
{
    int rc = -1;
    scoring_api_t dfu;
    bool x_in = excl;
    const string &dom = m_match->dom_subsystem ();

    tick ();
    rc = dom_dfv (meta, root, j.resources, &x_in, dfu);
    if (rc == 0) {
        int64_t score = dfu.overall_score ();
        unsigned int count = dfu.avail ();
        eval_edg_t ev_edg (count, count, excl);
        eval_egroup_t singleton;
        singleton.root = true;
        singleton.score = score;
        singleton.count = count;
        singleton.exclusive = excl;
        singleton.edges.push_back (ev_edg);
        dfu.add (dom, (*m_graph)[root].type, singleton);
        rc = resolve (root, j.resources, dfu, excl, needs);
    }
    return rc;
}

int dfu_impl_t::update (vtx_t root, jobmeta_t &meta, unsigned int needs,
                        bool exclusive)
{
    int rc = -1;
    map<string, int64_t> dfu;

    tick_color_base ();
    rc = upd_dfv (root, needs, exclusive, meta, dfu);
    return (rc > 0)? 0 : -1;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
