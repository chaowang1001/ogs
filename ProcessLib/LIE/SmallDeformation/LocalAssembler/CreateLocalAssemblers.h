/**
 * \copyright
 * Copyright (c) 2012-2017, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 */

#pragma once

#include <vector>

#include <logog/include/logog.hpp>

#include "NumLib/DOF/LocalToGlobalIndexMap.h"

#include "LocalDataInitializer.h"

namespace ProcessLib
{
namespace LIE
{
namespace SmallDeformation
{

namespace detail
{
template <unsigned GlobalDim, int DisplacementDim,
          template <typename, typename, unsigned, int>
          class LocalAssemblerMatrixImplementation,
          template <typename, typename, unsigned, int>
          class LocalAssemblerMatrixNearFractureImplementation,
          template <typename, typename, unsigned, int>
          class LocalAssemblerFractureImplementation,
          typename LocalAssemblerInterface, typename... ExtraCtorArgs>
void createLocalAssemblers(
    NumLib::LocalToGlobalIndexMap const& dof_table,
    std::vector<MeshLib::Element*> const& mesh_elements,
    std::vector<std::unique_ptr<LocalAssemblerInterface>>& local_assemblers,
    ExtraCtorArgs&&... extra_ctor_args)
{
    // Shape matrices initializer
    using LocalDataInitializer =
        LocalDataInitializer<LocalAssemblerInterface,
                             LocalAssemblerMatrixImplementation,
                             LocalAssemblerMatrixNearFractureImplementation,
                             LocalAssemblerFractureImplementation,
                             GlobalDim,
                             DisplacementDim, ExtraCtorArgs...>;

    DBUG("Create local assemblers.");
    // Populate the vector of local assemblers.
    local_assemblers.resize(mesh_elements.size());

    LocalDataInitializer initializer(dof_table);

    DBUG("Calling local assembler builder for all mesh elements.");
    GlobalExecutor::transformDereferenced(
        initializer,
        mesh_elements,
        local_assemblers,
        std::forward<ExtraCtorArgs>(extra_ctor_args)...);
}

}  // namespace detail

/*! Creates local assemblers for each element of the given \c mesh.
 *
 * \tparam LocalAssemblerImplementation the individual local assembler type
 * \tparam LocalAssemblerInterface the general local assembler interface
 * \tparam ExtraCtorArgs types of additional constructor arguments.
 *         Those arguments will be passed to the constructor of
 *         \c LocalAssemblerImplementation.
 *
 * The first two template parameters cannot be deduced from the arguments.
 * Therefore they always have to be provided manually.
 */
template <int DisplacementDim,
          template <typename, typename, unsigned, int>
          class LocalAssemblerMatrixImplementation,
          template <typename, typename, unsigned, int>
          class LocalAssemblerMatrixNearFractureImplementation,
          template <typename, typename, unsigned, int>
          class LocalAssemblerFractureImplementation,
          typename LocalAssemblerInterface, typename... ExtraCtorArgs>
void createLocalAssemblers(
    const unsigned dimension,
    std::vector<MeshLib::Element*> const& mesh_elements,
    NumLib::LocalToGlobalIndexMap const& dof_table,
    std::vector<std::unique_ptr<LocalAssemblerInterface>>& local_assemblers,
    ExtraCtorArgs&&... extra_ctor_args)
{
    DBUG("Create local assemblers.");

    switch (dimension)
    {
        case 2:
            detail::createLocalAssemblers<2, DisplacementDim,
                                          LocalAssemblerMatrixImplementation,
                                          LocalAssemblerMatrixNearFractureImplementation,
                                          LocalAssemblerFractureImplementation>(
                dof_table, mesh_elements, local_assemblers,
                std::forward<ExtraCtorArgs>(extra_ctor_args)...);
            break;
        case 3:
            detail::createLocalAssemblers<
                3, DisplacementDim, LocalAssemblerMatrixImplementation,
                LocalAssemblerMatrixNearFractureImplementation,
                LocalAssemblerFractureImplementation>(
                dof_table, mesh_elements, local_assemblers,
                std::forward<ExtraCtorArgs>(extra_ctor_args)...);
            break;
        default:
            OGS_FATAL(
                "Meshes with dimension different than two and three are not "
                "supported.");
    }
}

}  // namespace SmallDeformation
}  // namespace LIE
}  // namespace ProcessLib
