/**
 * \copyright
 * Copyright (c) 2012-2016, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#ifndef NUMLIB_TIMELOOP_H
#define NUMLIB_TIMELOOP_H

#include "MathLib/LinAlg/GlobalMatrixProviders.h"
#include "TimeDiscretizedODESystem.h"
#include "NonlinearSolver.h"


namespace NumLib
{

//! \addtogroup ODESolver
//! @{

/*! Integrate a single first-order ODE system over time.
 *
 * \tparam Matrix the type of matrices occuring in the linearization of the ODE.
 * \tparam Vector the type of the solution vector of the ODE.
 */
template<typename Matrix, typename Vector, NonlinearSolverTag NLTag>
class TimeLoopSingleODE final
{
public:
    using TDiscODESys  = TimeDiscretizedODESystemBase<Matrix, Vector, NLTag>;
    using LinearSolver = MathLib::LinearSolver<Matrix, Vector>;
    using NLSolver     = NonlinearSolver<Matrix, Vector, NLTag>;

    /*! Constructs an new instance.
     *
     * \param ode_sys The ODE system to be integrated
     * \param linear_solver the linear solver used to solve the linearized ODE system.
     * \param nonlinear_solver The solver to be used to resolve nonlinearities.
     */
    explicit
    TimeLoopSingleODE(TDiscODESys& ode_sys,
                      std::unique_ptr<LinearSolver>&& linear_solver,
                      std::unique_ptr<NLSolver>&& nonlinear_solver)
        : _ode_sys(ode_sys)
        , _linear_solver   (std::move(linear_solver))
        , _nonlinear_solver(std::move(nonlinear_solver))
    {}

    /*! Integrate the ODE from \c t0 to \c t_end with a timestep size of \c delta_t.
     *
     * The initial condition is \f$ x(\mathtt{t0}) = \mathtt{x0} \f$.
     *
     * After each timestep the callback \c post_timestep will be called,
     *  i.e., it won't be called with the initial condition as parameters.
     *
     * \tparam Callback Any callable object which can be called with the arguments of type
     *         \c double and <tt>Vector const&</tt> which contain the
     *         time and solution at the current timestep.
     *
     * \retval true  if the ODE could be successfully integrated
     * \retval false otherwise
     */
    template<typename Callback>
    bool loop(const double t0, const Vector x0,
              const double t_end, const double delta_t,
              Callback& post_timestep);

private:
    TDiscODESys& _ode_sys;
    std::unique_ptr<LinearSolver> _linear_solver;
    std::unique_ptr<NLSolver> _nonlinear_solver;
};

//! @}


template<typename Matrix, typename Vector, NonlinearSolverTag NLTag>
template<typename Callback>
bool
TimeLoopSingleODE<Matrix, Vector, NLTag>::
loop(const double t0, const Vector x0, const double t_end, const double delta_t,
     Callback& post_timestep)
{
    // solution vector
    Vector& x = MathLib::GlobalVectorProvider<Vector>::provider.getVector(x0);

    auto& time_disc = _ode_sys.getTimeDiscretization();

    time_disc.setInitialState(t0, x0); // push IC

    _nonlinear_solver->setEquationSystem(_ode_sys);

    if (time_disc.needsPreload()) {
        _nonlinear_solver->assemble(x);
        time_disc.pushState(t0, x0, _ode_sys); // TODO: that might do duplicate work
    }

    double t;
    unsigned timestep = 0;
    bool nl_slv_succeeded = true;
    for (t  = t0 + delta_t;
         t < t_end + std::numeric_limits<double>::epsilon();
         t = t0 + (timestep+1) * delta_t)
    {
        ++timestep;

        // INFO("time: %e, delta_t: %e", t, delta_t);
        time_disc.nextTimestep(t, delta_t);

        nl_slv_succeeded = _nonlinear_solver->solve(x);
        if (!nl_slv_succeeded) break;

        time_disc.pushState(t, x, _ode_sys);

        auto const  t_cb = t; // make sure the callback cannot overwrite anything.
        auto const& x_cb = x; // ditto.
        post_timestep(t_cb, x_cb);
    }

    MathLib::GlobalVectorProvider<Vector>::provider.releaseVector(x);

    if (!nl_slv_succeeded) {
        ERR("Nonlinear solver failed in timestep #%u at t = %g s", timestep, t);
    }
    return nl_slv_succeeded;
}

}

#endif // NUMLIB_TIMELOOP_H