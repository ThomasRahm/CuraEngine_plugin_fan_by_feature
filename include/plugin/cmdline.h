// BSD 4-Clause License
//
// Copyright (c) 2023, UltiMaker
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// 3. All advertising materials mentioning features or use of this software must
//   display the following acknowledgement:
//     This product includes software developed by UltiMaker.
//
// 4. Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY COPYRIGHT HOLDER "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef PLUGIN_CMDLINE_H
#define PLUGIN_CMDLINE_H

#include <fmt/compile.h>

#include <string_view>

namespace plugin::cmdline
{

constexpr std::string_view NAME = "CuraEngineFanSpeedByFeatureType";
constexpr std::string_view VERSION = "1.0.0";
static const auto VERSION_ID = fmt::format(FMT_COMPILE("{} {}"), NAME, VERSION);

constexpr std::string_view USAGE = R"({0}.
CuraEngine plugin for modifying fan speed by feature type

Usage:
  curaengine_plugin_fan_by_feature [--address <address>] [--port <port>]
  curaengine_plugin_fan_by_feature (-h | --help)
  curaengine_plugin_fan_by_feature --version

Options:
  -h --help                      Show this screen.
  --version                      Show version.
  -ip --address <address>        The IP address to connect the socket to [default: localhost].
  -p --port <port>               The port number to connect the socket to [default: 33800].
)";

} // namespace plugin::cmdline

#endif // PLUGIN_CMDLINE_H