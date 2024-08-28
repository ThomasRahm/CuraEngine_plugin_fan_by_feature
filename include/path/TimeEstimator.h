#include "cura/plugins/v0/gcodepath.pb.h"
#include <numeric>
#include "PathUtility.h"
#include "TimeMaterialEstimates.h"
#include <range/v3/view/enumerate.hpp>
#ifndef CURAENGINE_PLUGIN_FAN_BY_FEATURE_TIMEESTIMATOR_H
#define CURAENGINE_PLUGIN_FAN_BY_FEATURE_TIMEESTIMATOR_H

namespace fan_speed
{
using GCodePath = cura::plugins::v0::GCodePath;
using Point2D = cura::plugins::v0::Point2D;

struct AllTimeEstimates
{
    TimeMaterialEstimates all_paths_estimate;
    std::vector<TimeMaterialEstimates> path_estimates;
    std::vector<std::vector<double>> point_estimates;

};

class TimeEstimate{
public:
//Copied from CuraEngine but removed some material estimation code. Required for minimum layer time fan speed settings.
static AllTimeEstimates computeNaiveTimeEstimates(const std::vector<const cura::plugins::v0::GCodePath*> paths, const plugin::Settings& settings, int extruder_nr)
{
        AllTimeEstimates estimates;
        if(paths.empty())
        {
            return estimates;
        }
        Point2D p0;
        if(paths.front()->path().path_size() > 0)
        {
            p0 = paths.front()->path().path(0);
        }
        double slowest_path_speed = std::accumulate(
            paths.begin(),
            paths.end(),
            std::numeric_limits<double>::max(),
            [](double value, const GCodePath* path)
            {
                return PathUtils::featureIsTravel(path->feature()) ? value : std::min(value, path->speed_derivatives().velocity() * path->speed_factor());
            });

        bool was_retracted = false; // wrong assumption; won't matter that much. (TODO)
        estimates.path_estimates = std::vector<TimeMaterialEstimates>(paths.size());
        estimates.point_estimates = std::vector<std::vector<double>>(paths.size());

        for (auto [path_idx, path] : ranges::views::enumerate(paths))
        {
            bool is_extrusion_path = false;
            double* path_time_estimate;

            estimates.path_estimates[path_idx].extrude_time_at_minimum_speed = 0.0;
            estimates.path_estimates[path_idx].extrude_time_at_slowest_path_speed = 0.0;
            estimates.point_estimates[path_idx] = std::vector<double>(paths[path_idx]->path().path_size());
            if (! PathUtils::featureIsTravel(path->feature()))
            {
                is_extrusion_path = true;
                path_time_estimate = &estimates.path_estimates[path_idx].extrude_time;
            }
            else
            {
                if (path->retract())
                {
                    path_time_estimate = &estimates.path_estimates[path_idx].retracted_travel_time;
                }
                else
                {
                    path_time_estimate = &estimates.path_estimates[path_idx].unretracted_travel_time;
                }
                if (path->retract() != was_retracted)
                { // handle retraction times
                    double retract_unretract_time;
                    const double retraction_distance = settings.get<double>("retraction_amount",extruder_nr, path->mesh_name());
                    if (path->retract())
                    {
                        retract_unretract_time = retraction_distance /settings.get<double>("retraction_retract_speed",extruder_nr, path->mesh_name()) ;
                    }
                    else
                    {
                        retract_unretract_time = retraction_distance / settings.get<double>("retraction_prime_speed",extruder_nr, path->mesh_name());
                    }
                    estimates.path_estimates[path_idx].retracted_travel_time += 0.5 * retract_unretract_time;
                    estimates.path_estimates[path_idx].unretracted_travel_time += 0.5 * retract_unretract_time;
                }
            }
            for (int64_t point_idx = 0; point_idx<path->path().path_size();point_idx++)
            {
                Point2D p1 = path->path().path(point_idx);
                double length = double(PathUtils::vSize(p0, p1))/1000.0;
                if (is_extrusion_path)
                {
                    if (length > 0)
                    {
                        estimates.path_estimates[path_idx].extrude_time_at_minimum_speed += length / settings.get<double>("cool_min_speed",extruder_nr, path->mesh_name());
                        estimates.path_estimates[path_idx].extrude_time_at_slowest_path_speed += length / slowest_path_speed;
                    }
                }
                double thisTime = length / (path->speed_derivatives().velocity() * path->speed_factor());
                *path_time_estimate += thisTime;
                estimates.point_estimates[path_idx][point_idx] = thisTime;
                p0 = p1;
            }
            estimates.all_paths_estimate += estimates.path_estimates[path_idx];
        }
        return estimates;
    }
};
} //namespace fan_speed

#endif // CURAENGINE_PLUGIN_FAN_BY_FEATURE_TIMEESTIMATOR_H
