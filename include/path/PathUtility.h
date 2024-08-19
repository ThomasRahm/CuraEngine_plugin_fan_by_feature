#include "cura/plugins/v0/polygons.pb.h"
#include "cura/plugins/v0/printfeatures.pb.h"


#ifndef CURAENGINE_PLUGIN_FAN_BY_FEATURE_PATHUTILITY_H
#define CURAENGINE_PLUGIN_FAN_BY_FEATURE_PATHUTILITY_H
namespace fan_speed
{
using PrintFeature = cura::plugins::v0::PrintFeature;
using Point2D = cura::plugins::v0::Point2D;

class PathUtils
{
public:

    static bool featureIsTravel(PrintFeature feature)
    {
        return feature == 0 || feature == 8 || feature == 9;
    }

    static double vSize(Point2D a, Point2D b)
    {
        double x_delta = a.x() - b.x();
        double y_delta = a.y() - b.y();
        return sqrt(x_delta * x_delta + y_delta * y_delta);
    }
};
} // namespace fan_speed
#endif // CURAENGINE_PLUGIN_FAN_BY_FEATURE_PATHUTILITY_H
