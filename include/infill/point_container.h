// Copyright (c) 2023 UltiMaker
// CuraEngine is released under the terms of the AGPLv3 or higher

#ifndef UTILS_GEOMETRY_POINT_CONTAINER_H
#define UTILS_GEOMETRY_POINT_CONTAINER_H

#include "infill/concepts.h"

#include <polyclipping/clipper.hpp>
#include <range/v3/view/drop.hpp>
#include <range/v3/view/transform.hpp>

#include <initializer_list>
#include <memory>
#include <vector>

namespace infill::geometry
{

using Point = ClipperLib::IntPoint;

/*! The base clase of all point based container types
 *
 * @tparam P
 * @tparam IsClosed
 * @tparam Direction
 * @tparam Container
 */
template<concepts::point P>
struct point_container : public std::vector<P>
{
    constexpr point_container() noexcept = default;
    constexpr explicit point_container(std::initializer_list<P> points) noexcept
        : std::vector<P>(points)
    {
    }
};




#endif // UTILS_GEOMETRY_POINT_CONTAINER_H