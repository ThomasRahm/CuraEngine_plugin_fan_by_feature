#include "cura/plugins/v0/gcodepath.pb.h"
#include "path/PathUtility.h"
#include "path/TimeEstimator.h"
#include "plugin/broadcast.h"
#include "plugin/metadata.h"
#include "plugin/settings.h"

#include <boost/asio/awaitable.hpp>
#include <polyclipping/clipper.hpp>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/reverse.hpp>
#include <spdlog/spdlog.h>

#if __has_include(<coroutine>)
#include <coroutine>
#elif __has_include(<experimental/coroutine>)
#include <experimental/coroutine>
#define USE_EXPERIMENTAL_COROUTINE
#endif
#include <filesystem>
#include <memory>
#include <vector>
#ifndef CURAENGINE_PLUGIN_FAN_BY_FEATURE_FANSPEED_H
#define CURAENGINE_PLUGIN_FAN_BY_FEATURE_FANSPEED_H

namespace fan_speed
{
using LayerIndex = int64_t;
using Point2D = cura::plugins::v0::Point2D;
using PrintFeature = cura::plugins::v0::PrintFeature;
using GCodePath = cura::plugins::v0::GCodePath;
class FanSpeedCalculator
{
private:
    std::vector<std::vector<std::pair<size_t, double>>> fanSpeedCache;
    LayerIndex layer_idx;
    int remaining_extruder;
    bool is_raft_layer;
    plugin::Settings settings;
    std::unordered_map<PrintFeature, std::string> fan_speed_map;
    AllTimeEstimates estimates;
    int extruder_nr = 0;

    double applyInitialLayersFanSpeedModification(double fan_speed)
    {
        const int64_t cool_fan_full_layer = settings.get<int64_t>("cool_fan_full_layer", extruder_nr, "");
        const double cool_fan_speed_0 = settings.get<double>("cool_fan_speed_0", extruder_nr, "");

        if (layer_idx < cool_fan_full_layer && cool_fan_full_layer > 0 // don't apply initial layer fan speed speedup if disabled.
            && ! is_raft_layer // don't apply initial layer fan speed speedup to raft, but to model layers
        )
        {
            // Slow down the fan on the layers below the [cool_fan_full_layer], where layer 0 is speed 0.
            fan_speed = cool_fan_speed_0 + (fan_speed - cool_fan_speed_0) * std::max(LayerIndex(0), layer_idx) / cool_fan_full_layer;
        }
        return fan_speed;
    }

    double getFanSpeedOfPath(const GCodePath* path)
    {
        PrintFeature feature = path->feature();
        if (fan_speed_map.contains(feature))
        {
            // todo cache with feature/model as key?
            return settings.get<double>(fan_speed_map.at(feature), extruder_nr, path->mesh_name());
        }
        return -1;
    }

public:
    FanSpeedCalculator(plugin::Settings& settings, std::vector<const cura::plugins::v0::GCodePath*> paths, LayerIndex layer_idx, int remaining_extruder, int extruder_nr)
        : settings(settings)
        , layer_idx(layer_idx)
        , remaining_extruder(remaining_extruder)
        , is_raft_layer(layer_idx < 0)
        , extruder_nr(extruder_nr)
    {
        if (is_raft_layer)
        {
            // Raft fan speeds are handled in the Engine.
            return;
        }

        fan_speed_map = { { PrintFeature::NONETYPE, "cool_fan_speed_min" },
                          { PrintFeature::OUTERWALL, "cool_fan_speed_wall_0" },
                          { PrintFeature::INNERWALL, "cool_fan_speed_wall_x" },
                          { PrintFeature::SKIN, "cool_fan_speed_skin" },
                          { PrintFeature::SUPPORT, "cool_fan_speed_support" },
                          { PrintFeature::SKIRTBRIM, "cool_fan_speed_support" },
                          { PrintFeature::INFILL, "cool_fan_speed_infill" },
                          { PrintFeature::SUPPORTINFILL, "cool_fan_speed_support" },
                          { PrintFeature::SUPPORTINTERFACE, "cool_fan_speed_support_interface" } };

        estimates = TimeEstimate::computeNaiveTimeEstimates(paths, settings, extruder_nr);


        enum class MinimumLayerTimeValidity
        {
            ONLY_OUTER,
            ALL,
            NEVER,
            LEGACY
        };
        std::string mlt_string = "all"; // Todo make this a setting?

        MinimumLayerTimeValidity apply_mlt_to
            = (mlt_string == "only_outer")
                ? MinimumLayerTimeValidity::ONLY_OUTER
                : (mlt_string == "all" ? MinimumLayerTimeValidity::ALL : (mlt_string == "legacy" ? MinimumLayerTimeValidity::LEGACY : MinimumLayerTimeValidity::NEVER));

        fanSpeedCache = std::vector<std::vector<std::pair<size_t, double>>>(std::max(paths.size(), size_t(1)));
        double initial_fan_speed
            = (paths.size() == 0 || paths.front()->fan_speed() < 0) ? -1 : applyInitialLayersFanSpeedModification(getFanSpeedOfPath(paths.front()));
        fanSpeedCache[0].emplace_back(0, initial_fan_speed);
        double current_fan_speed = initial_fan_speed;
        // As a few for loops count down use a signed integer for the index to prevent issues when comparing with signed integers.
        int64_t last_set_path_idx = 0;
        int64_t last_set_point_idx = 0;
        int64_t final_extrusion_path_idx = 0;
        bool added_non_travel_cooling = paths.size() == 0 ? false : ! PathUtils::featureIsTravel(paths.front()->feature());
        double first_extrude_fan_speed = -1;

        for (int64_t path_idx = 1; path_idx < paths.size(); path_idx++)
        {
            const std::string mesh_name = paths[path_idx]->mesh_name();
            double max_spin_up_time = settings.get<double>("cool_fan_spin_up_time", extruder_nr, mesh_name);
            double max_spin_down_time = settings.get<double>("cool_fan_spin_down_time", extruder_nr, mesh_name);
            // Iterate through paths until fan speed changed, then go back for spin time
            double path_fan_speed = applyInitialLayersFanSpeedModification(getFanSpeedOfPath(paths[path_idx]));
            if (path_fan_speed < 0)
            {
                path_fan_speed = -1;
            }

            if ((paths[path_idx]->feature() == cura::plugins::v0::PrintFeature::OUTERWALL && apply_mlt_to == MinimumLayerTimeValidity::ONLY_OUTER)
                || apply_mlt_to == MinimumLayerTimeValidity::ALL || (apply_mlt_to == MinimumLayerTimeValidity::LEGACY && remaining_extruder <= 0))
            {
                double fan_speed_difference
                    = std::max(applyInitialLayersFanSpeedModification(settings.get<double>("cool_fan_speed_max", extruder_nr, mesh_name)) - path_fan_speed, 0.0);
                double mlt_curve_position = calculateMinimumLayerTimeCurvePosition();
                path_fan_speed = path_fan_speed + mlt_curve_position * fan_speed_difference;
            }

            if (path_fan_speed != current_fan_speed && ! PathUtils::featureIsTravel(paths[path_idx]->feature()))
            {
                bool added = false;
                std::pair<size_t, size_t> add_location(path_idx, 0);
                double spin_time = (std::abs(path_fan_speed - current_fan_speed) / 100.0) * ((path_fan_speed > current_fan_speed) ? max_spin_up_time : max_spin_down_time);
                double accumulated_time = 0;
                if (spin_time > 0)
                {
                    // look at previous paths to determine where the fan speed has to be changed to be correct for this feature.
                    for (int64_t path_idx_back = path_idx - 1; path_idx_back >= last_set_path_idx && ! added; path_idx_back--)
                    {
                        for (int64_t point_idx = paths[path_idx_back]->path().path_size() - 1;
                             ((point_idx >= 0 && path_idx_back > last_set_path_idx) || point_idx >= last_set_point_idx) && ! added;
                             point_idx--)
                        {
                            const bool path_back_is_travel = PathUtils::featureIsTravel(paths[path_idx]->feature());
                            const double previous_accumulated_time = accumulated_time;

                            // semi-arbitrary weight factor. Goal is to roughly distribute the error between both fan speeds (the fan speed before and the one of the current path)
                            const double spin_factor
                                = 1.0 / (1.0 - (((path_fan_speed > current_fan_speed) ? max_spin_up_time : max_spin_down_time) / (max_spin_up_time + max_spin_down_time)));

                            // Use previous location as the estimates are time to a point
                            accumulated_time += estimates.point_estimates[add_location.first][add_location.second]
                                              * (! path_back_is_travel ? spin_factor : 1);
                            if (accumulated_time >= spin_time)
                            {
                                added = true;
                            }

                            // If changing it now creates a larger error than changing it one vertex later, change the fan speed one vertex later
                            if (accumulated_time < spin_time || (accumulated_time > spin_time && spin_time - previous_accumulated_time > accumulated_time - spin_time)
                                || path_back_is_travel)
                            {
                                add_location = std::pair<size_t, size_t>(path_idx_back, point_idx);
                            }
                        }
                    }
                }
                else
                {
                    add_location = std::pair<size_t, size_t>(path_idx, 0);
                }

                if (! added_non_travel_cooling)
                {
                    fanSpeedCache[0][0].second = path_fan_speed;
                }
                else if (last_set_path_idx > add_location.first || (last_set_path_idx == add_location.first && last_set_point_idx >= add_location.second))
                {
                    fanSpeedCache[last_set_path_idx].back().second = path_fan_speed;
                }
                else
                {
                    fanSpeedCache[add_location.first].emplace_back(add_location.second, path_fan_speed);
                    last_set_path_idx = add_location.first;
                    last_set_point_idx = add_location.second;
                }

                current_fan_speed = path_fan_speed;
            }
            added_non_travel_cooling |= ! PathUtils::featureIsTravel(paths[path_idx]->feature());
            if (! PathUtils::featureIsTravel(paths[path_idx]->feature()))
            {
                final_extrusion_path_idx = path_idx;
                if (first_extrude_fan_speed == -1)
                {
                    first_extrude_fan_speed = path_fan_speed;
                }
            }
        }


        if (remaining_extruder == -1 && first_extrude_fan_speed != -1)
        {
            // assume the next layer will start with the same fan speed that this layer had started with. This would not be true if multiple extruders are used, but it is extruder
            // switching may take longer than spinning up the part cooling fans anyway.
            fanSpeedCache[final_extrusion_path_idx].emplace_back(paths[final_extrusion_path_idx]->path().path_size() - 1, first_extrude_fan_speed);
        }


        // Ensure every vertex of every path has a valid fan speed. The fan speed of a vertex is valid if there is a specific fan speed for it,
        // or a previous vertex of the path has a valid fan speed (In which case the fan speed has not changed since this previous vertex)
        for (int64_t path_idx = 1; path_idx < paths.size(); path_idx++)
        {
            if (fanSpeedCache[path_idx].empty())
            {
                fanSpeedCache[path_idx].emplace_back(0, fanSpeedCache[path_idx - 1].back().second);
            }
            else if (fanSpeedCache[path_idx].front().first != 0)
            {
                fanSpeedCache[path_idx].insert(fanSpeedCache[path_idx].begin(), std::pair<size_t, double>(0, fanSpeedCache[path_idx - 1].back().second));
            }
        }
    }

    double calculateMinimumLayerTimeCurvePosition()
    {
        const int64_t cool_fan_full_layer = settings.get<int64_t>("cool_fan_full_layer", extruder_nr, "");
        const double cool_min_layer_time = settings.get<double>("cool_min_layer_time", extruder_nr, "");
        const double cool_min_layer_time_fan_speed_max = settings.get<double>("cool_min_layer_time_fan_speed_max", extruder_nr, "");

        if ((layer_idx < cool_fan_full_layer && cool_fan_full_layer > 0) // don't apply MinimumLayerTime fan speedup on the first few layers.
            || is_raft_layer) // Raft layers are still handled in the engine
        {
            return 0.0;
        }

        /*
                   min layer time
                   :
                   :  min layer time fan speed min
                |  :  :
      ^    max..|__:  :
                |  \  :
     fan        |   \ :
    speed  min..|... \:___________
                |________________
                  layer time >
        */
        // interpolate fan speed (for cool_fan_full_layer and for cool_min_layer_time_fan_speed_max)
        double fan_speed_mlt_curve_position = 0.0;
        if (estimates.all_paths_estimate.getTotalTime() < cool_min_layer_time)
        {
            fan_speed_mlt_curve_position = 1.0;
        }
        else if (cool_min_layer_time >= cool_min_layer_time_fan_speed_max)
        {
            fan_speed_mlt_curve_position = 0.0;
        }
        else if (estimates.all_paths_estimate.getTotalTime() < cool_min_layer_time_fan_speed_max)
        {
            double layer_time_diff = cool_min_layer_time_fan_speed_max - cool_min_layer_time;
            double fraction_of_slope = (estimates.all_paths_estimate.getTotalTime() - cool_min_layer_time) / layer_time_diff;
            fan_speed_mlt_curve_position = 1 - fraction_of_slope;
        }

        if (fan_speed_mlt_curve_position > 1 || fan_speed_mlt_curve_position < 0)
        {
            spdlog::warn("Fan speed adjustment for minimum layer time threshold lies outside of expected range.");
        }
        return fan_speed_mlt_curve_position;
    }


    double getFanSpeed(size_t path_idx, size_t vertex_idx) const
    {
        if (path_idx < fanSpeedCache.size())
        {
            for (auto [idx, fan_speed_pair] : fanSpeedCache[path_idx] | ranges::views::enumerate | ranges::views::reverse)
            {
                if (fan_speed_pair.first <= vertex_idx)
                {
                    return fan_speed_pair.second;
                }
            }
        }
        else if (! is_raft_layer)
        {
            spdlog::warn("Fan speed for previously non existing path requested");
        }
        return -1;
    }
};

} // namespace fan_speed

#endif // CURAENGINE_PLUGIN_FAN_BY_FEATURE_FANSPEED_H
