
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

#ifndef DFU_MATCH_CB_HPP
#define DFU_MATCH_CB_HPP

#include <set>
#include <map>
#include <vector>
#include <cstdint>
#include <iostream>
#include "resource_graph.hpp"
#include "matcher_data.hpp"
#include "scoring_api.hpp"
#include "jobspec.hpp"
#include "../planner/Planner/planner.h"

namespace resource_model {

/*! Base DFU matcher class.
 *  Define the set of visitor methods that are called
 *  back by a DFU resource-graph traverser.
 */
class dfu_match_cb_t : public matcher_data_t
{
public:
    dfu_match_cb_t () : m_trav_level (0) { }
    dfu_match_cb_t (const std::string &name)
        : matcher_data_t (name), m_trav_level (0) { }
    dfu_match_cb_t (const dfu_match_cb_t &o)
        : matcher_data_t (o)
    {
        m_trav_level = o.m_trav_level;
    }
    dfu_match_cb_t &operator= (const dfu_match_cb_t &o)
    {
        matcher_data_t::operator= (o);
        m_trav_level = o.m_trav_level;
        return *this;
    }
    virtual ~dfu_match_cb_t () { }

    /*!
     *  Called back when all of the graph has been visited.
     *  Please see README.SCORING_HOWTO that details our scoring interface.
     *
     *  \param subsystem
     *               subsystem_t for the dominant subsystem.
     *  \param resources
     *               vector of resources to be matched.
     *  \param g     filtered resource graph.
     *  \param dfu   score interface object - map of the sorted vectors
     *               of the scores computed from depth-first walks under
     *               vertex u on dominant subsystems and up-walks on all
     *               of the selected auxiliary subsystems. This map is keyed
     *               off of a subsystem_t object.
     *
     *  \return      return 0 on success; otherwise -1.
     */
    virtual int dom_finish_graph (const subsystem_t &subsystem,
                                  const std::vector<Resource> &resources,
                                  const f_resource_graph_t &g, scoring_api_t &dfu)
    {
        return 0;
    }

    /*!
     *  Called back on each preorder visit of the dominant subsystem.
     *
     *  \param u     descriptor of the visiting vertex.
     *  \param subsystem
     *               subsystem_t for the dominant subsystem.
     *  \param resources
     *               vector of resources to be matched.
     *  \param g     filtered resource graph.
     *
     *  \return      return 0 on success; otherwise -1.
     */
    virtual int dom_discover_vtx (vtx_t u, const subsystem_t &subsystem,
                                  const std::vector<Resource> &resources,
                                  const f_resource_graph_t &g)
    {
        m_trav_level++;
        return 0;
    }

    /*!
     *  Called back on each postorder visit of the dominant subsystem. Should
     *  return a score calculated based on the subtree and up walks using the score
     *  API object (dfu). Any score aboved MATCH_MET is qualified to be a match.
     *  Please see README.SCORING_HOWTO that details our scoring interface.
     *
     *  \param u     descriptor of the visiting vertex
     *  \param subsystem
     *               subsystem_t for the dominant subsystem
     *  \param resources
     *               vector of resources to be matched
     *  \param g     filtered resource graph
     *  \param dfu   score interface object - map of the sorted vectors
     *               of the scores computed from depth-first walks under
     *               vertex u on dominant subsystems and up-walks on all
     *               of the selected auxiliary subsystems. This map is keyed
     *               off of a subsystem_t object.
     *
     *  \return      return 0 on success; otherwise -1
     */
    virtual int dom_finish_vtx (vtx_t u, const subsystem_t &subsystem,
                                const std::vector<Resource> &resources,
                                const f_resource_graph_t &g,
                                scoring_api_t &dfu)
    {
        m_trav_level--;
        return 0;
    }

    /*! Called back on each pre-up visit of an auxiliary subsystem.
     *
     *  \param u     descriptor of the visiting vertex
     *  \param subsystem
     *               subsytem_t of the auxiliary subsystem being walked
     *  \param resources
     *               vector of resources to be matched
     *  \param g     filtered resource graph
     *
     *  \return      return 0 on success; otherwise -1
     */
    virtual int aux_discover_vtx (vtx_t u, const subsystem_t &subsystem,
                                  const std::vector<Resource> &resources,
                                  const f_resource_graph_t &g)

    {
        m_trav_level++;
        return 0;
    }

    /*
     *  Called back on each post-up visit of the auxiliary subsystem. Should
     *  return a score calculated based on the subtree and up walks using the score
     *  API object (dfu). Any score aboved MATCH_MET is qualified to be a match.
     *  Please see README.SCORING_HOWTO that details our scoring interface.
     *
     *  \param u     descriptor of the visiting vertex
     *  \param subsystem
     *               subsytem_t info for an auxiliary subsystem
     *  \param resources
     *               vector of resources to be matched
     *  \param g     filtered resource graph object
     *  \param dfu
     *               score interface object - map of the sorted vectors
     *               of the scores computed from depth-first walks under
     *               vertex u on dominant subsystems and up-walks on all
     *               of the selected auxiliary subsystems. This map is keyed
     *               off of a subsystem_t object
     *  \return      return 0 on success; otherwise -1
     */
    virtual int aux_finish_vtx (vtx_t u, const subsystem_t &subsystem,
                                const std::vector<Resource> &resources,
                                const f_resource_graph_t &g, scoring_api_t &dfu)
    {
        m_trav_level--;
        return 0;
    }

    void incr ()
    {
        m_trav_level++;
    }
    void decr ()
    {
        m_trav_level--;
    }
    std::string level ()
    {
        int i;
        std::string prefix = "";
        for (i = 0; i < m_trav_level; ++i)
            prefix += "----";
        return prefix;
    }

private:
    int m_trav_level;
};
}

#endif // DFU_MATCH_CB_HPP

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
