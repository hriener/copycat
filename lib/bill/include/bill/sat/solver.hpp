/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
*------------------------------------------------------------------------------------------------*/
#pragma once

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-else"
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-comparison"
#pragma GCC diagnostic ignored "-Wunused-label"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wzero-length-array"
#include <sat_solvers/ghack.hpp>
#include <sat_solvers/glucose.hpp>
#include <sat_solvers/maple.hpp>
#pragma GCC diagnostic pop
#else
#pragma warning(push)
#pragma warning(disable:4571)
#pragma warning(disable:4625)
#pragma warning(disable:4626)
#pragma warning(disable:4710)
#pragma warning(disable:4774)
#pragma warning(disable:4820)
#include <sat_solvers/ghack.hpp>
#include <sat_solvers/glucose.hpp>
#include <sat_solvers/maple.hpp>
#pragma warning(pop)
#endif

#include "types.hpp"

#include <memory>
#include <variant>
#include <vector>

namespace bill {

class result {
public:
	using model_type = std::vector<lbool_type>;
	using clause_type = std::vector<lit_type>;

	enum class states : uint8_t {
		satisfiable,
		unsatisfiable,
		undefined,
		timeout,
		dirty,
	};

	static std::string to_string( states const& state )
	{
		switch (state)
		{
		case states::satisfiable:
		  return "satisfiable";
		case states::unsatisfiable:
		  return "unsatisfiable";
		case states::timeout:
		  return "timeout";
		case states::dirty:
		  return "dirty";
		case states::undefined:
		default:
		  return "undefined";
		}
	}

#pragma region Constructors
	result(states state = states::undefined)
	    : state_(state)
	{}

	result(model_type const& model)
	    : state_(states::satisfiable)
	    , data_(model)
	{}

	result(clause_type const& unsat_core)
	    : state_(states::unsatisfiable)
	    , data_(unsat_core)
	{}
#pragma endregion

#pragma region Properties
	inline bool is_satisfiable() const
	{
		return (state_ == states::satisfiable);
	}

	inline bool is_unsatisfiable() const
	{
		return (state_ == states::unsatisfiable);
	}

	inline bool is_undefined() const
	{
		return (state_ == states::undefined);
	}

	inline model_type model() const
	{
		return std::get<model_type>(data_);
	}
#pragma endregion

#pragma region Overloads
	inline operator bool() const
	{
		return (state_ == states::satisfiable);
	}

	inline explicit operator std::string() const
	{
		return result::to_string(state_);
	}
#pragma endregion

private:
	states state_;
	std::variant<model_type, clause_type> data_;
};

enum class solvers {
	glucose_41,
	ghack,
	maple,
};

template<solvers Solver = solvers::maple>
class solver;

template<>
class solver<solvers::glucose_41> {
	using solver_type = Glucose::Solver;

public:
#pragma region Constructors
	solver()
	    : solver_(std::make_unique<solver_type>())
	{}
#pragma endregion

#pragma region Modifiers
	void restart()
	{
		solver_.reset();
		solver_ = std::make_unique<solver_type>();
		state_ = result::states::undefined;
	}

	var_type add_variable()
	{
		return solver_->newVar();
	}

	void add_variables(uint32_t num_variables = 1)
	{
		for (auto i = 0u; i < num_variables; ++i) {
			solver_->newVar();
		}
	}

	auto add_clause(std::vector<lit_type> const& clause)
	{
		Glucose::vec<Glucose::Lit> literals;
		for (auto lit : clause) {
			literals.push(Glucose::mkLit(lit.variable(), lit.is_complemented()));
		}
		auto const result = solver_->addClause_(literals);
		state_ = result ? result::states::dirty : result::states::unsatisfiable;
		return result;
	}

	auto add_clause(lit_type lit)
	{
		auto const result = solver_->addClause(
		    Glucose::mkLit(lit.variable(), lit.is_complemented()));
		state_ = result ? result::states::dirty : result::states::unsatisfiable;
		return result;
	}

	result get_model() const
	{
		assert(state_ == result::states::satisfiable);
		result::model_type model;
		for (auto i = 0; i < solver_->model.size(); ++i) {
			if (solver_->model[i] == Glucose::l_False) {
				model.emplace_back(lbool_type::false_);
			} else if (solver_->model[i] == Glucose::l_True) {
				model.emplace_back(lbool_type::true_);
			} else {
				model.emplace_back(lbool_type::undefined);
			}
		}
		return result(model);
	}

	result get_core() const
	{
		assert(state_ == result::states::unsatisfiable);
		result::clause_type unsat_core;
		for (auto i = 0; i < solver_->conflict.size(); ++i) {
			unsat_core.emplace_back(Glucose::var(solver_->conflict[i]),
			                        Glucose::sign(solver_->conflict[i]) ?
			                            negative_polarity :
			                            positive_polarity);
		}
		return result(unsat_core);
	}

	result get_result() const
	{
		assert(state_ != result::states::dirty);
		if (state_ == result::states::satisfiable) {
			return get_model();
		} else if (state_ == result::states::unsatisfiable) {
			return get_core();
		} else {
			return result();
		}
	}

	result::states solve(std::vector<lit_type> const& assumptions = {},
	                     uint32_t conflict_limit = 0)
	{
		if (state_ != result::states::dirty) {
			return state_;
		}

		assert(solver_->okay() == true);
		if (conflict_limit) {
			solver_->setConfBudget(conflict_limit);
		}

		Glucose::vec<Glucose::Lit> literals;
		for (auto lit : assumptions) {
			literals.push(Glucose::mkLit(lit.variable(), lit.is_complemented()));
		}

		Glucose::lbool state = solver_->solveLimited(literals);
		if (state == Glucose::l_True) {
			state_ = result::states::satisfiable;
		} else if (state == Glucose::l_False) {
			state_ = result::states::unsatisfiable;
		} else {
			state_ = result::states::undefined;
		}
		return state_;
	}
#pragma endregion

#pragma region Properties
	uint32_t num_variables() const
	{
		return solver_->nVars();
	}

	uint32_t num_clauses() const
	{
		return solver_->nClauses();
	}
#pragma endregion

private:
	/*! \brief Backend solver */
	std::unique_ptr<solver_type> solver_;

	/*! \brief Current state of the solver */
	result::states state_ = result::states::undefined;
};

template<>
class solver<solvers::ghack> {
	using solver_type = GHack::Solver;

public:
#pragma region Constructors
	solver()
	    : solver_(std::make_unique<solver_type>())
	{}
#pragma endregion

#pragma region Modifiers
	void restart()
	{
		solver_.reset();
		solver_ = std::make_unique<solver_type>();
		state_ = result::states::undefined;
	}

	var_type add_variable()
	{
		return solver_->newVar();
	}

	void add_variables(uint32_t num_variables = 1)
	{
		for (auto i = 0u; i < num_variables; ++i) {
			solver_->newVar();
		}
	}

	auto add_clause(std::vector<lit_type> const& clause)
	{
		GHack::vec<GHack::Lit> literals;
		for (auto lit : clause) {
			literals.push(GHack::mkLit(lit.variable(), lit.is_complemented()));
		}
		auto const result = solver_->addClause_(literals);
		state_ = result ? result::states::dirty : result::states::unsatisfiable;
		return result;
	}

	auto add_clause(lit_type lit)
	{
		auto const result = solver_->addClause(
		    GHack::mkLit(lit.variable(), lit.is_complemented()));
		state_ = result ? result::states::dirty : result::states::unsatisfiable;
		return result;
	}

	result get_model() const
	{
		assert(state_ == result::states::satisfiable);
		result::model_type model;
		for (auto i = 0; i < solver_->model.size(); ++i) {
			if (solver_->model[i] == GHack::l_False) {
				model.emplace_back(lbool_type::false_);
			} else if (solver_->model[i] == GHack::l_True) {
				model.emplace_back(lbool_type::true_);
			} else {
				model.emplace_back(lbool_type::undefined);
			}
		}
		return result(model);
	}

	result get_core() const
	{
		assert(state_ == result::states::unsatisfiable);
		result::clause_type unsat_core;
		for (auto i = 0; i < solver_->conflict.size(); ++i) {
			unsat_core.emplace_back(GHack::var(solver_->conflict[i]),
			                        GHack::sign(solver_->conflict[i]) ?
			                            negative_polarity :
			                            positive_polarity);
		}
		return result(unsat_core);
	}

	result get_result() const
	{
		assert(state_ != result::states::dirty);
		if (state_ == result::states::satisfiable) {
			return get_model();
		} else if (state_ == result::states::unsatisfiable) {
			return get_core();
		} else {
			return result();
		}
	}

	result::states solve(std::vector<lit_type> const& assumptions = {},
	                     uint32_t conflict_limit = 0)
	{
		if (state_ != result::states::dirty) {
			return state_;
		}

		assert(solver_->okay() == true);
		if (conflict_limit) {
			solver_->setConfBudget(conflict_limit);
		}

		GHack::vec<GHack::Lit> literals;
		for (auto lit : assumptions) {
			literals.push(GHack::mkLit(lit.variable(), lit.is_complemented()));
		}

		GHack::lbool state = solver_->solveLimited(literals);
		if (state == GHack::l_True) {
			state_ = result::states::satisfiable;
		} else if (state == GHack::l_False) {
			state_ = result::states::unsatisfiable;
		} else {
			state_ = result::states::undefined;
		}
		return state_;
	}
#pragma endregion

#pragma region Properties
	uint32_t num_variables() const
	{
		return solver_->nVars();
	}

	uint32_t num_clauses() const
	{
		return solver_->nClauses();
	}
#pragma endregion

private:
	/*! \brief Backend solver */
	std::unique_ptr<solver_type> solver_;

	/*! \brief Current state of the solver */
	result::states state_ = result::states::undefined;
};

template<>
class solver<solvers::maple> {
	using solver_type = Maple::Solver;

public:
#pragma region Constructors
	solver()
	    : solver_(std::make_unique<solver_type>())
	{}
#pragma endregion

#pragma region Modifiers
	void restart()
	{
		solver_.reset();
		solver_ = std::make_unique<solver_type>();
		state_ = result::states::undefined;
	}

	var_type add_variable()
	{
		return solver_->newVar();
	}

	void add_variables(uint32_t num_variables = 1)
	{
		for (auto i = 0u; i < num_variables; ++i) {
			solver_->newVar();
		}
	}

	auto add_clause(std::vector<lit_type> const& clause)
	{
		Maple::vec<Maple::Lit> literals;
		for (auto lit : clause) {
			literals.push(Maple::mkLit(lit.variable(), lit.is_complemented()));
		}
		auto const result = solver_->addClause_(literals);
		state_ = result ? result::states::dirty : result::states::unsatisfiable;
		return result;
	}

	auto add_clause(lit_type lit)
	{
		auto const result = solver_->addClause(
		    Maple::mkLit(lit.variable(), lit.is_complemented()));
		state_ = result ? result::states::dirty : result::states::unsatisfiable;
		return result;
	}

	result get_model() const
	{
		assert(state_ == result::states::satisfiable);
		result::model_type model;
		for (auto i = 0; i < solver_->model.size(); ++i) {
			if (solver_->model[i] == Maple::l_False) {
				model.emplace_back(lbool_type::false_);
			} else if (solver_->model[i] == Maple::l_True) {
				model.emplace_back(lbool_type::true_);
			} else {
				model.emplace_back(lbool_type::undefined);
			}
		}
		return result(model);
	}

	result get_core() const
	{
		assert(state_ == result::states::unsatisfiable);
		result::clause_type unsat_core;
		for (auto i = 0; i < solver_->conflict.size(); ++i) {
			unsat_core.emplace_back(Maple::var(solver_->conflict[i]),
			                        Maple::sign(solver_->conflict[i]) ?
			                            negative_polarity :
			                            positive_polarity);
		}
		return result(unsat_core);
	}

	result get_result() const
	{
		assert(state_ != result::states::dirty);
		if (state_ == result::states::satisfiable) {
			return get_model();
		} else if (state_ == result::states::unsatisfiable) {
			return get_core();
		} else {
			return result();
		}
	}

	result::states solve(std::vector<lit_type> const& assumptions = {},
	                     uint32_t conflict_limit = 0)
	{
		if (state_ != result::states::dirty) {
			return state_;
		}

		assert(solver_->okay() == true);
		if (conflict_limit) {
			solver_->setConfBudget(conflict_limit);
		}

		Maple::vec<Maple::Lit> literals;
		for (auto lit : assumptions) {
			literals.push(Maple::mkLit(lit.variable(), lit.is_complemented()));
		}

		Maple::lbool state = solver_->solveLimited(literals);
		if (state == Maple::l_True) {
			state_ = result::states::satisfiable;
		} else if (state == Maple::l_False) {
			state_ = result::states::unsatisfiable;
		} else {
			state_ = result::states::undefined;
		}
		return state_;
	}
#pragma endregion

#pragma region Properties
	uint32_t num_variables() const
	{
		return solver_->nVars();
	}

	uint32_t num_clauses() const
	{
		return solver_->nClauses();
	}
#pragma endregion

private:
	/*! \brief Backend solver */
	std::unique_ptr<solver_type> solver_;

	/*! \brief Current state of the solver */
	result::states state_ = result::states::dirty;
};

} // namespace bill
