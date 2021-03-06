
#pragma once

#include "interpolations/shape_function.hpp"
#include "interpolations/interpolation_factory.hpp"
#include "quadrature/quadrature_factory.hpp"
#include "quadrature/minimum_degree.hpp"
#include "io/json_forward.hpp"

namespace neon::fem
{
/// integral models a finite element type integral in the Galerkin finite element
/// method.
template <typename Interpolation, typename Quadrature, int DerivativeOrder, int RankCorrection = 0>
class integral
{
public:
    static_assert(Interpolation::spatial_dimension == Quadrature::spatial_dimension,
                  "The Interpolation and Quadrature type must be templated over the same space");

public:
    /// Order of the derivative in the expression
    static constexpr auto derivative_order = DerivativeOrder;
    /// Manual correction for rank-insufficient matrices in integral
    static constexpr auto rank_correction = RankCorrection;
    /// Type alias for Interpolation
    using interpolation_type = Interpolation;
    /// Type alias for Quadrature
    using quadrature_type = Quadrature;
    /// Fix the size of the shape function derivative to the size of the quadrature points
    using value_type = typename interpolation_type::value_type;

public:
    integral(element_topology const topology, json const& element_options)
        : m_shape_function{interpolation_factory<interpolation_type>::make(topology)},
          m_quadrature{quadrature_factory<
              quadrature_type>::make(topology,
                                     minimum_degree(m_shape_function->polynomial_order(),
                                                    m_shape_function->monomial_order(),
                                                    derivative_order),
                                     element_options)},
          m_values(m_shape_function->evaluate(m_quadrature->coordinates()))
    {
    }

    /// Perform the numerical integration of a lambda function.
    /// \param integral Value for the numerical integration (accumulated into)
    /// \param f A lambda function that accepts an femValue and quadrature point
    /// \return The numerically integrated matrix
    template <typename MatrixType, typename Callable>
    void integrate(MatrixType& integral, Callable&& f) const
    {
        std::size_t indices = m_quadrature->points();
        auto const& weights = m_quadrature->weights();

        for (std::size_t index{0}; index < indices; ++index)
        {
            integral.noalias() += f(m_values[index], index) * weights[index];
        }
    }

    /// Perform the numerical integration of a lambda function.
    /// \param integral Value for the numerical integration (accumulated into)
    /// \param f A lambda function that accepts an femValue and quadrature point
    /// \return The numerically integrated matrix
    template <typename MatrixType, typename Callable>
    void integrate(Eigen::Map<MatrixType> integral, Callable&& f) const
    {
        std::size_t indices = m_quadrature->points();
        auto const& weights = m_quadrature->weights();

        for (std::size_t index{0}; index < indices; ++index)
        {
            integral.noalias() += f(m_values[index], index) * weights[index];
        }
    }

    /// Evaluate the function \p function at each integration point
    /// \tparam A callable type
    template <typename Callable>
    void for_each(Callable&& function) const
    {
        std::size_t indices = m_quadrature->points();

        for (std::size_t index{0}; index < indices; ++index)
        {
            function(m_values[index], index);
        }
    }

    auto quadrature() const -> quadrature_type const& { return *m_quadrature; }

    auto shape_function() const -> interpolation_type const& { return *m_shape_function; }

protected:
    std::unique_ptr<interpolation_type> m_shape_function;

    std::unique_ptr<quadrature_type> m_quadrature;
    /// Shape functions and their derivatives evaluated at the quadrature points
    std::vector<value_type> m_values;
};

}
