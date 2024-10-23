#ifndef PLUGIN_GENERATE_H
#define PLUGIN_GENERATE_H

#include "path/FanSpeed.h"
#include "plugin/broadcast.h"
#include "plugin/metadata.h"
#include "plugin/settings.h"

#include <boost/asio/awaitable.hpp>
#include <spdlog/spdlog.h>

#if __has_include(<coroutine>)
#include <coroutine>
#elif __has_include(<experimental/coroutine>)
#include <experimental/coroutine>
#define USE_EXPERIMENTAL_COROUTINE
#endif
#include <filesystem>
#include <memory>

namespace plugin
{

using Service = cura::plugins::slots::gcode_paths::v0::modify::GCodePathsModifyService::AsyncService;
using Rsponse = cura::plugins::slots::gcode_paths::v0::modify::CallResponse;
struct ModifyPaths
{
    using ServicePtr = std::shared_ptr<cura::plugins::slots::gcode_paths::v0::modify::GCodePathsModifyService::AsyncService>;
    ServicePtr generate_service{ std::make_shared<cura::plugins::slots::gcode_paths::v0::modify::GCodePathsModifyService::AsyncService>() };
    Broadcast::shared_settings_t settings{ std::make_shared<Broadcast::settings_t>() };
    std::shared_ptr<Metadata> metadata{ std::make_shared<Metadata>() };
    boost::asio::awaitable<void> run()
    {
        while (true)
        {
            grpc::ServerContext server_context;
            grpc::Status status = grpc::Status::OK;
            cura::plugins::slots::gcode_paths::v0::modify::CallRequest request;
            grpc::ServerAsyncResponseWriter<Rsponse> writer{ &server_context };
            co_await agrpc::request(&Service::RequestCall, *generate_service, server_context, request, writer, boost::asio::use_awaitable);

            if (! status.ok())
            {
                co_await agrpc::finish_with_error(writer, status, boost::asio::use_awaitable);
                continue;
            }


            cura::plugins::slots::gcode_paths::v0::modify::CallResponse response;

            try
            {
                spdlog::debug("Processing Layer {} Path count {}",request.layer_nr(), request.gcode_paths_size());
                std::vector<const cura::plugins::v0::GCodePath*> paths;
                for (int path_idx = 0; path_idx < request.gcode_paths_size(); path_idx++)
                {
                    paths.emplace_back(&request.gcode_paths(path_idx));
                }

                auto client_metadata = getUuid(server_context);

                Settings extruder_settings = settings.get()->at(client_metadata);

                fan_speed::FanSpeedCalculator fan_calc(extruder_settings, paths, request.layer_nr(), 0, request.extruder_nr());
                double previous_fan_speed = fan_calc.getFanSpeed(0, 0);
                for (int path_idx = 0; path_idx < request.gcode_paths_size(); path_idx++)
                {
                    const cura::plugins::v0::GCodePath& path = request.gcode_paths(path_idx);
                    int path_size = path.path().path_size();
                    cura::plugins::v0::GCodePath* current_path = response.add_gcode_paths();
                    current_path->CopyFrom(path);
                    current_path->clear_path();
                    previous_fan_speed = fan_calc.getFanSpeed(path_idx, 0);
                    current_path->set_fan_speed(previous_fan_speed);
                    spdlog::debug("Layer {} Path {} Type {}: Set actual fan speed to {}",request.layer_nr(), path_idx, int(paths[path_idx]->feature()), previous_fan_speed);

                    for (int point_idx = 0; point_idx < path_size; point_idx++)
                    {
                        auto point = path.path().path(point_idx);
                        double fan_speed = fan_calc.getFanSpeed(path_idx, point_idx);
                        if (fan_speed != previous_fan_speed)
                        {
                            current_path = response.add_gcode_paths();
                            current_path->CopyFrom(path);
                            current_path->clear_path();
                            current_path->set_fan_speed(fan_speed);
                            previous_fan_speed = fan_speed;
                            spdlog::debug("Layer {} Path {} Point {}: Set actual fan speed to {}",request.layer_nr(), path_idx, point_idx, previous_fan_speed);
                            current_path->set_retract(false);
                        }
                        auto* new_point = current_path->mutable_path()->add_path();
                        new_point->set_x(point.x());
                        new_point->set_y(point.y());
                        new_point->set_z(point.z());
                    }
                }
            }
            catch (const std::exception& e)
            {
                spdlog::error("Error: {}", e.what());
                status = grpc::Status(grpc::StatusCode::INTERNAL, static_cast<std::string>(e.what()));
            }

            if (! status.ok())
            {
                co_await agrpc::finish_with_error(writer, status, boost::asio::use_awaitable);
                continue;
            }

            co_await agrpc::finish(writer, response, status, boost::asio::use_awaitable);
        }
    }
};

} // namespace plugin

#endif // PLUGIN_GENERATE_H
