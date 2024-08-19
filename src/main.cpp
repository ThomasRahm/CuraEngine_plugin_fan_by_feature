
#include "cura/plugins/slots/gcode_paths/v0/modify.grpc.pb.h"
#include "cura/plugins/slots/gcode_paths/v0/modify.pb.h"
#include "plugin/cmdline.h" // Custom command line argument definitions
#include "plugin/handshake.h" // Handshake interface
#include "plugin/plugin.h" // Plugin interface

#include <boost/asio/signal_set.hpp>
#include <docopt/docopt.h> // Library for parsing command line arguments
#include <fmt/format.h> // Formatting library
#include <grpcpp/server.h>
#include <spdlog/spdlog.h> // Logging library

#include <map>

int main(int argc, const char** argv)
{
    spdlog::set_level(spdlog::level::info);
    constexpr bool show_help = true;
    const std::map<std::string, docopt::value> args
        = docopt::docopt(fmt::format(plugin::cmdline::USAGE, plugin::cmdline::NAME), { argv + 1, argv + argc }, show_help, plugin::cmdline::VERSION_ID);

    plugin::Plugin<plugin::ModifyPaths> plugin{ args.at("--address").asString(), args.at("--port").asString(), grpc::InsecureServerCredentials() };
    plugin.addHandshakeService(plugin::Handshake{ .metadata = plugin.metadata, .broadcast_subscriptions = { cura::plugins::v0::SlotID::SETTINGS_BROADCAST } });
    auto broadcast_settings = std::make_shared<plugin::Broadcast::settings_t>();
    plugin.addBroadcastService(plugin::Broadcast{ .settings = broadcast_settings, .metadata = plugin.metadata });
    plugin.addGenerateService(plugin::ModifyPaths{ .settings = broadcast_settings, .metadata = plugin.metadata });
    plugin.start();
    plugin.run();
    plugin.stop();
}