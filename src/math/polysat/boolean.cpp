/*++
Copyright (c) 2021 Microsoft Corporation

Module Name:

    polysat boolean variables

Author:

    Jakob Rath 2021-04-6

--*/
#include "math/polysat/boolean.h"
#include "math/polysat/clause.h"
#include "math/polysat/log.h"

namespace polysat {

    sat::bool_var bool_var_manager::new_var() {
        sat::bool_var var;
        if (m_unused.empty()) {
            var = size();
            m_value.push_back(l_undef);
            m_value.push_back(l_undef);
            m_level.push_back(UINT_MAX);
            m_deps.push_back(null_dependency);
            m_kind.push_back(kind_t::unassigned);
            m_clause.push_back(nullptr);
            m_watch.push_back({});
            m_watch.push_back({});
            m_activity.push_back(0);
        }
        else {
            var = m_unused.back();
            m_unused.pop_back();
            SASSERT_EQ(m_level[var], UINT_MAX);
            SASSERT_EQ(m_value[2*var], l_undef);
            SASSERT_EQ(m_value[2*var+1], l_undef);
            SASSERT_EQ(m_kind[var], kind_t::unassigned);
            SASSERT_EQ(m_clause[var], nullptr);
            SASSERT_EQ(m_deps[var], null_dependency);
        }
        return var;
    }

    void bool_var_manager::del_var(sat::bool_var var) {
        SASSERT(std::count(m_unused.begin(), m_unused.end(), var) == 0);
        auto lit = sat::literal(var);
        m_value[lit.index()]    = l_undef;
        m_value[(~lit).index()] = l_undef;
        m_level[var] = UINT_MAX;
        m_kind[var] = kind_t::unassigned;
        m_clause[var] = nullptr;
        m_deps[var] = null_dependency;
        m_watch[lit.index()].reset();
        m_watch[(~lit).index()].reset();
        m_free_vars.del_var_eh(var);
        // TODO: this is disabled for now, since re-using variables for different constraints may be confusing during debugging. Should be enabled later.
        // m_unused.push_back(var);
    }

    void bool_var_manager::propagate(sat::literal lit, unsigned lvl, clause& reason) {
        LOG("Propagate literal " << lit << " @ " << lvl << " by " << reason);
        assign(kind_t::bool_propagation, lit, lvl, &reason);
        SASSERT(is_bool_propagation(lit));
    }

    void bool_var_manager::decide(sat::literal lit, unsigned lvl, clause& lemma) {
        LOG("Decide literal " << lit << " @ " << lvl);
        assign(kind_t::decision, lit, lvl, &lemma);
        SASSERT(is_decision(lit));
    }

    void bool_var_manager::decide(sat::literal lit, unsigned lvl) {
        LOG("Decide literal " << lit << " @ " << lvl);
        assign(kind_t::decision, lit, lvl, nullptr);
        SASSERT(is_decision(lit));
    }

    void bool_var_manager::eval(sat::literal lit, unsigned lvl) {
        LOG("Eval literal " << lit << " @ " << lvl);
        assign(kind_t::value_propagation, lit, lvl, nullptr);
        SASSERT(is_value_propagation(lit));
    }

    void bool_var_manager::asserted(sat::literal lit, unsigned lvl, dependency dep) {
        LOG("Asserted " << lit << " @ " << lvl);
        assign(dep == null_dependency ? kind_t::decision : kind_t::assumption, lit, lvl, nullptr, dep);
        SASSERT(is_decision(lit) || is_assumption(lit));
    }

    void bool_var_manager::assign(kind_t k, sat::literal lit, unsigned lvl, clause* reason, dependency dep) {
        SASSERT(!is_assigned(lit));
        SASSERT(k != kind_t::unassigned);
        m_value[lit.index()] = l_true;
        m_value[(~lit).index()] = l_false;
        m_level[lit.var()] = lvl;
        m_kind[lit.var()] = k;
        m_clause[lit.var()] = reason;
        m_deps[lit.var()] = dep;
        m_free_vars.del_var_eh(lit.var());
    }

    void bool_var_manager::unassign(sat::literal lit) {
        SASSERT(is_assigned(lit));
        m_value[lit.index()] = l_undef;
        m_value[(~lit).index()] = l_undef;
        m_level[lit.var()] = UINT_MAX;
        m_kind[lit.var()] = kind_t::unassigned;
        m_clause[lit.var()] = nullptr;
        m_deps[lit.var()] = null_dependency;
        m_free_vars.unassign_var_eh(lit.var());
    }

    std::ostream& bool_var_manager::display(std::ostream& out) const {
        for (sat::bool_var v = 0; v < size(); ++v) {
            sat::literal lit(v);
            if (value(lit) == l_true)
                out << " " << lit;
            else if (value(lit) == l_false)
                out << " " << ~lit;
        }
        return out;
    }
}
