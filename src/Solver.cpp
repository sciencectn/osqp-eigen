/**
 * @file Solver.cpp
 * @author Giulio Romualdi
 * @copyright Released under the terms of the LGPLv2.1 or later, see LGPL.TXT
 * @date 2018
 */

// OsqpEigen
#include <OsqpEigen/Data.hpp>
#include <OsqpEigen/Settings.hpp>
#include <OsqpEigen/Solver.hpp>

OsqpEigen::Solver::Solver()
    : m_isSolverInitialized(false)
{
    m_settings = std::make_unique<OsqpEigen::Settings>();
    m_data = std::make_unique<OsqpEigen::Data>();
    m_workspace = nullptr;
}

OsqpEigen::Solver::~Solver()
{
    clearSolver();
}

bool OsqpEigen::Solver::clearSolverVariables()
{
    if(!m_isSolverInitialized){
        std::cerr << "[OsqpEigen::Solver::clearSolverVariables] Unable to clear the solver variables. "
                  << "Are you sure that the solver is initialized?"
                  << std::endl;
        return false;
    }

    for(int i = 0; i < m_workspace->data->n; i++){
        m_workspace->x[i] = 0;
        m_workspace->x_prev[i] = 0;

        m_workspace->Px[i] = 0;
        m_workspace->Aty[i] = 0;
        m_workspace->Atdelta_y[i] = 0;

        m_workspace->delta_x[i] = 0;
        m_workspace->Pdelta_x[i] = 0;
    }

    for(int i = 0; i < m_workspace->data->m; i++){
        m_workspace->z[i] = 0;
        m_workspace->z_prev[i] = 0;
        m_workspace->y[i] = 0;

        m_workspace->Ax[i] = 0;
        m_workspace->delta_y[i] = 0;

        m_workspace->Adelta_x[i] = 0;
    }

    for(int i = 0; i < m_workspace->data->n + m_workspace->data->m; i++){
        m_workspace->xz_tilde[i] = 0;
    }


    return true;
}

bool OsqpEigen::Solver::initSolver()
{
    if(m_isSolverInitialized){
        std::cerr << "[OsqpEigen::Solver::initSolver] The solver has been already initialized. "
                  << "Please use clearSolver() method to deallocate memory."
                  << std::endl;
        return false;
    }

    if(!m_data->isSet()){
        std::cerr << "[OsqpEigen::Solver::initSolver] Some data are not set."
                  << std::endl;
        return false;
    }

    if(osqp_setup(&m_workspace, m_data->getData(),
                  m_settings->getSettings()) != 0 ){
        std::cerr << "[OsqpEigen::Solver::initSolver] Unable to setup the workspace."
                  << std::endl;
        return false;
    }


    m_isSolverInitialized = true;
    return true;
}

bool OsqpEigen::Solver::isInitialized()
{
    return m_isSolverInitialized;
}

void OsqpEigen::Solver::clearSolver()
{
    if(m_isSolverInitialized){
        osqp_cleanup(m_workspace);
        m_isSolverInitialized = false;
    }
}

bool OsqpEigen::Solver::solve()
{
    if(!m_isSolverInitialized){
        std::cerr << "[OsqpEigen::Solver::solve] The solve has hot been initialized yet. "
                  << "Please call initSolver() method."
                  << std::endl;
        return false;
    }

    if(osqp_solve(m_workspace) != 0){
        std::cerr << "[OsqpEigen::Solver::solve] Unable to solve the problem."
                  << std::endl;
        return false;
    }

    // check if the solution is feasible
    if(m_workspace->info->status_val != OSQP_SOLVED)
    {
        std::cerr << "[OsqpEigen::Solver::solve] The solution is unfeasible."
                  << std::endl;
        return false;
    }

    return true;
}

const Eigen::VectorXd &OsqpEigen::Solver::getSolution()
{
    // copy data from an array to Eigen vector
    c_float* solution = m_workspace->solution->x;
    m_solution = Eigen::Map<Eigen::VectorXd>(solution, m_workspace->data->n, 1);

    return m_solution;
}

const std::unique_ptr<OsqpEigen::Settings>& OsqpEigen::Solver::settings() const
{
    return m_settings;
}

const std::unique_ptr<OsqpEigen::Data>& OsqpEigen::Solver::data() const
{
    return m_data;
}

bool OsqpEigen::Solver::updateGradient(const Eigen::Ref<const Eigen::Matrix<c_float, Eigen::Dynamic, 1>>& gradient)
{
    // check if the dimension of the gradient is correct
    if(gradient.rows() != m_workspace->data->n){
        std::cerr << "[OsqpEigen::Solver::updateGradient] The size of the gradient must be equal to the number of the variables."
                  << std::endl;
        return false;
    }

    // update the gradient vector
    if(osqp_update_lin_cost(m_workspace, gradient.data())){
        std::cerr << "[OsqpEigen::Solver::updateGradient] Error when the update gradient is called."
                  << std::endl;
        return false;
    }
    return true;
}

bool OsqpEigen::Solver::updateLowerBound(const Eigen::Ref<const Eigen::Matrix<c_float, Eigen::Dynamic, 1>>& lowerBound)
{
    // check if the dimension of the lowerBound vector is correct
    if(lowerBound.rows() != m_workspace->data->m){
        std::cerr << "[OsqpEigen::Solver::updateLowerBound] The size of the lower bound must be equal to the number of the variables."
                  << std::endl;
        return false;
    }

    // update the lower bound vector
    if(osqp_update_lower_bound(m_workspace, lowerBound.data())){
        std::cerr << "[OsqpEigen::Solver::updateLowerBound] Error when the update lower bound is called."
                  << std::endl;
        return false;
    }

    return true;
}

bool OsqpEigen::Solver::updateUpperBound(const Eigen::Ref<const Eigen::Matrix<c_float, Eigen::Dynamic, 1>>& upperBound)
{
    // check if the dimension of the upperBound vector is correct
    if(upperBound.rows() != m_workspace->data->m){
        std::cerr << "[OsqpEigen::Solver::updateUpperBound] The size of the upper bound must be equal to the number of the variables."
                  << std::endl;
        return false;
    }

    // update the upper bound vector
    if(osqp_update_upper_bound(m_workspace, upperBound.data())){
        std::cerr << "[OsqpEigen::Solver::updateUpperBound] Error when the update upper bound is called."
                  << std::endl;
        return false;
    }
    return true;
}

bool OsqpEigen::Solver::updateBounds(const Eigen::Ref<const Eigen::Matrix<c_float, Eigen::Dynamic, 1>>& lowerBound,
                                     const Eigen::Ref<const Eigen::Matrix<c_float, Eigen::Dynamic, 1>>& upperBound)
{
    // check if the dimension of the upperBound vector is correct
    if(upperBound.rows() != m_workspace->data->m){
        std::cerr << "[OsqpEigen::Solver::updateBounds] The size of the upper bound must be equal to the number of the variables."
                  << std::endl;
        return false;
    }

    // check if the dimension of the lowerBound vector is correct
    if(lowerBound.rows() != m_workspace->data->m){
        std::cerr << "[OsqpEigen::Solver::updateBounds] The size of the lower bound must be equal to the number of the variables."
                  << std::endl;
        return false;
    }

    // update lower and upper constraints
    if(osqp_update_bounds(m_workspace, lowerBound.data(), upperBound.data())){
        std::cerr << "[OsqpEigen::Solver::updateBounds] Error when the update bounds is called."
                  << std::endl;
        return false;
    }
    return true;
}
