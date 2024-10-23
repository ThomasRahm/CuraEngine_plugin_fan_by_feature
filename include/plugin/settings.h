#ifndef PLUGIN_SETTINGS_H
#define PLUGIN_SETTINGS_H

#include "cura/plugins/slots/broadcast/v0/broadcast.grpc.pb.h"

#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/reverse.hpp>

#include <algorithm>
#include <cctype>
#include <ctre.hpp>
#include <locale>
#include <optional>
#include <semver.hpp>
#include <string>
#include <unordered_map>

namespace plugin
{

struct Settings
{


    inline static const std::vector<std::string> loaded_settings {
        "cool_fan_speed_min",
        "cool_fan_speed_wall_0",
        "cool_fan_speed_wall_x",
        "cool_fan_speed_skin",
        "cool_fan_speed_infill",
        "cool_fan_speed_support",
        "cool_fan_speed_support_interface",
        "cool_fan_spin_up_time",
        "cool_fan_spin_down_time",
        "cool_fan_speed_0",
        "cool_min_layer_time_fan_speed_max",
        "cool_fan_full_layer",
        "retraction_retract_speed",
        "retraction_prime_speed",
        "retraction_amount",
        "cool_min_speed",
        "cool_min_layer_time",
        "cool_fan_speed_max",
        "cool_fan_mlt_mode"
    };

    // map[setting_key] => pair(extruder_values,model_values)
    std::unordered_map<std::string, std::pair<std::vector<std::string>,std::unordered_map<std::string,std::string>>> setting_storage;


    explicit Settings(const cura::plugins::slots::broadcast::v0::BroadcastServiceSettingsRequest& request, const std::shared_ptr<Metadata>& metadata)
    {

        for (const std::string& key : loaded_settings)
        {
            setting_storage.emplace(key, std::pair<std::vector<std::string>, std::unordered_map<std::string, std::string>>());
            setting_storage[key].first.resize(request.extruder_settings_size());

            for (const auto& object : request.object_settings())
            {
                std::optional<std::string> object_name = retrieveSettings("mesh_name", object, metadata);
                if (object_name.has_value())
                {
                    std::optional<std::string> setting_value = retrieveSettings(key, object, metadata);
                    if (setting_value.has_value())
                    {
                        setting_storage[key].second[object_name.value()] = setting_value.value();
                        spdlog::debug("Found per object settings for mesh {} {}:{}",object_name.value(),key,setting_value.value());
                    }
                }
            }

            for (int extruder_idx = 0; extruder_idx < request.extruder_settings_size(); extruder_idx++)
            {
                std::optional<std::string> setting_value = retrieveSettings(key, extruder_idx, request, metadata);
                if (setting_value.has_value())
                {
                    setting_storage[key].first[extruder_idx] = setting_value.value();
                    spdlog::debug("Found extruder setting for extruder {} {}:{}",extruder_idx,key,setting_value.value());
                }
            }
        }


    }

    template<typename A>
    A get(const std::string& key, int extruder,const std::string& model) const;



    [[maybe_unused]] static std::optional<std::string>
        retrieveSettings(const std::string& settings_key, const cura::plugins::slots::broadcast::v0::Settings& settings, const auto& metadata)
    {
        const auto settings_key_ = settingKey(settings_key, metadata->plugin_name, metadata->plugin_version);

        if (settings.settings().contains(settings_key_))
        {
            return settings.settings().at(settings_key_);
        }

        if (settings.settings().contains(settings_key))
        {
            return settings.settings().at(settings_key);
        }

        return std::nullopt;
    }

    [[maybe_unused]] static std::optional<std::string> retrieveSettings(
        const std::string& settings_key,
        const size_t extruder_nr,
        const cura::plugins::slots::broadcast::v0::BroadcastServiceSettingsRequest& request,
        const auto& metadata)
    {
        const auto& settings = request.extruder_settings().at(static_cast<int>(extruder_nr));
        std::optional<std::string> result = retrieveSettings(settings_key, settings, metadata);
        if (result.has_value())
        {
            return result;
        }

        return retrieveSettings(settings_key, request.global_settings(), metadata); // May return default values instead of setting value. Bug?
    }



    static bool validatePlugin(const cura::plugins::slots::handshake::v0::CallRequest& request, const std::shared_ptr<Metadata>& metadata)
    {
        if (request.plugin_name() == metadata->plugin_name && request.plugin_version() == metadata->plugin_version)
        {
            return true;
        }
        return false;
    }

    static std::string settingKey(std::string_view short_key, std::string_view name, std::string_view version)
    {
        std::string lower_name{ name };
        auto semantic_version = semver::from_string(version);
        std::transform(
            lower_name.begin(),
            lower_name.end(),
            lower_name.begin(),
            [](const auto& c)
            {
                return std::tolower(c);
            });
        return fmt::format("_plugin__{}__{}_{}_{}__{}", lower_name, semantic_version.major, semantic_version.minor, semantic_version.patch, short_key);
    }
};
// Defined out of struct as  error: explicit specialization in non-namespace scope when using GCC. See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85282
template<>
std::string Settings::get<std::string>(const std::string& key, int extruder, const std::string& model) const
{
    if (setting_storage.contains(key) && setting_storage.at(key).second.contains(model))
    {
        return setting_storage.at(key).second.at(model);
    }
    else if (setting_storage.contains(key) && extruder < setting_storage.at(key).first.size())
    {
        return setting_storage.at(key).first[extruder];
    }
    else
    {
        spdlog::error("Could not find setting: {}", key);
        return "";
    }
}

template<>
double Settings::get<double>(const std::string& key, int extruder, const std::string& model) const
{
    return std::stod(get<std::string>(key, extruder, model));
}

template<>
int64_t Settings::get<int64_t>(const std::string& key, int extruder, const std::string& model) const
{
    return std::stoi(get<std::string>(key, extruder, model));
}


} // namespace plugin

#endif // PLUGIN_SETTINGS_H
