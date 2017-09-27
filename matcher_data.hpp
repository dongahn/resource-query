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

#ifndef MATCHER_DATA_HPP
#define MATCHER_DATA_HPP

#include <string>
#include <iostream>
#include <vector>
#include <set>
#include <map>
#include "jobspec.hpp"
#include "../planner/Planner/planner.h"

namespace resource_model {

enum match_score_t {
    MATCH_UNMET = 0,
    MATCH_MET = 1
};

enum match_op_t {
    MATCH_ALLOCATE,
    MATCH_ALLOCATE_ORELSE_RESERVE
};

/*! Base matcher data class.
 *  Provides idioms to specify the target subsystems and
 *  resource relationship types which then allow resource
 *  module to filter the graph data store for the match
 *  callback object
 */
class matcher_data_t
{
public:
    matcher_data_t () : m_name ("anonymous") { }
    matcher_data_t (const std::string &name) : m_name (name) { }
    matcher_data_t (const matcher_data_t &o)
    {
       sdau_resource_types = o.sdau_resource_types;
       m_name = o.m_name;
       m_subsystems = o.m_subsystems;
       m_subsystems_map = o.m_subsystems_map;
    }
    matcher_data_t &operator=(const matcher_data_t &o)
    {
       sdau_resource_types = o.sdau_resource_types;
       m_name = o.m_name;
       m_subsystems = o.m_subsystems;
       m_subsystems_map = o.m_subsystems_map;
       return *this;
    }
    ~matcher_data_t ()
    {
        sdau_resource_types.clear ();
        m_subsystems.clear ();
        m_subsystems_map.clear ();
    }

    /*! Add a subsystem and the relationship type that this resource base matcher
     *  will use. Vertices and edges of the resource graph are filtered in based
     *  on this information. Each vertex and edge that is a part of this subsystem
     *  and relationship type will be selected.
     *
     *  This method must be called at least once to set the dominant subsystem
     *  to use. This method can be called multiple times with a distinct subsystem,
     *  each becomes an auxiliary subsystem. The queuing order can be reconstructed
     *  get_subsystems.
     *
     *  \param s     a subsystem to select
     *  \param tf    edge (or relation type) to select. pass * for selecting all types
     *  \param m     matcher_t
     *  \return      0 on success; -1 on an error
     */
    int add_subsystem (const subsystem_t s, const std::string tf = "*")
    {
        if (m_subsystems_map.find (s) == m_subsystems_map.end ()) {
            m_subsystems.push_back (s);
            m_subsystems_map[s].insert (tf);
            return 0;
        }
        return -1;
    }
    const std::string &matcher_name ()
    {
        return m_name;
    }
    void set_matcher_name (const std::string &name)
    {
        m_name = name;
    }
    const std::vector<subsystem_t> &subsystems () const
    {
        return m_subsystems;
    }

    /*
     * \return      return the dominant subsystem this matcher has selected to use
     */
    const subsystem_t &dom_subsystem () const
    {
        return *(m_subsystems.begin());
    }

    /*
     * \return      return the subsystem selector to be used for graph filtering
     */
    const multi_subsystemsS &subsystemsS () const
    {
        return m_subsystems_map;
    }

    std::map<subsystem_t, std::set<std::string> > sdau_resource_types;

    unsigned int select_count (const Resource &resource,
                               unsigned int qc) const
    {
        unsigned int count = resource.count.max;
        if (count < resource.count.min || resource.count.min > qc)
            return 0;

        switch (resource.count.oper) {
        case '+':
            while (count > qc)
                count -= resource.count.operand;
            break;
        case '*':
            while (count > qc)
                count /= resource.count.operand;
            break;
        case '^':
            while (count > qc)
                count = prev_pwr (resource.count.min,
                                  count, resource.count.operand);
            break;
        default:
            break;
        }
        return (count >= resource.count.min)? count : 0;
    }


private:
    const unsigned prev_pwr (unsigned min, unsigned count,
                             unsigned operand) const
    {
       unsigned int i = min;
       unsigned int sol = 0;
       while (i < count) {
           sol = i;
           for (int j = 0; j < operand; j++)
               i *= i;
       }
       return sol;
    }

    std::string m_name;
    std::vector<subsystem_t> m_subsystems;
    multi_subsystemsS m_subsystems_map;
};
}
#endif // MATCHER_DATA_HPP

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
