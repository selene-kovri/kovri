/**                                                                                           //
 * Copyright (c) 2015-2017, The Kovri I2P Router Project                                      //
 *                                                                                            //
 * All rights reserved.                                                                       //
 *                                                                                            //
 * Redistribution and use in source and binary forms, with or without modification, are       //
 * permitted provided that the following conditions are met:                                  //
 *                                                                                            //
 * 1. Redistributions of source code must retain the above copyright notice, this list of     //
 *    conditions and the following disclaimer.                                                //
 *                                                                                            //
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list     //
 *    of conditions and the following disclaimer in the documentation and/or other            //
 *    materials provided with the distribution.                                               //
 *                                                                                            //
 * 3. Neither the name of the copyright holder nor the names of its contributors may be       //
 *    used to endorse or promote products derived from this software without specific         //
 *    prior written permission.                                                               //
 *                                                                                            //
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY        //
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF    //
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL     //
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,       //
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,               //
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS    //
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,          //
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF    //
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.               //
 */

#include "app/instance.h"

// Config parsing
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/program_options.hpp>

// Logging
#include <boost/core/null_deleter.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include <cstdint>
#include <stdexcept>
#include <memory>
#include <vector>

#include "client/util/parse.h"

#include "core/crypto/aes.h"
#include "core/crypto/rand.h"
#include "core/util/filesystem.h"

// TODO(anonimal): instance configuration should probably be moved to libcore

namespace kovri {
namespace app {

namespace bpo = boost::program_options;

void Configuration::ParseKovriConfig() {
  // Random generated port if none is supplied via CLI or config
  // See: i2p.i2p/router/java/src/net/i2p/router/transport/udp/UDPEndpoint.java
  // TODO(unassigned): move this elsewhere (outside of ParseArgs()) when possible
  std::uint16_t port = kovri::core::RandInRange32(9111, 30777);
  // Default visible option
  bpo::options_description help("\nhelp");
  help.add_options()("help,h", "");  // Blank so we can use custom message above
  // Map options values from command-line and config
  bpo::options_description system("\nsystem");
  system.add_options()(
      "host", bpo::value<std::string>()->default_value("127.0.0.1"))(
      "port,p", bpo::value<int>()->default_value(port))(
      "data-dir",
      bpo::value<std::string>()->default_value(
          kovri::core::GetDefaultDataPath().string()))(
      "daemon,d", bpo::value<bool>()->default_value(false))(
      "service,s", bpo::value<std::string>()->default_value(""))(
      "log-to-console", bpo::value<bool>()->default_value(true))(
      "log-to-file", bpo::value<bool>()->default_value(true))(
      "log-file-name", bpo::value<std::string>()->default_value(""))(
      // TODO(anonimal): use only 1 log file?
      // Log levels
      // 0 = fatal
      // 1 = error fatal
      // 2 = warn error fatal
      // 3 = info warn error fatal
      // 4 = debug info warn error fatal
      // 5 = trace debug info warn error fatal
      "log-level", bpo::value<std::uint16_t>()->default_value(3))(
      "kovriconf,c", bpo::value<std::string>()->default_value(""))(
      "tunnelsconf,t", bpo::value<std::string>()->default_value(""));
  // This is NOT our default values for log-file-name, kovriconf and tunnelsconf

  bpo::options_description network("\nnetwork");
  network.add_options()
    ("v6,6", bpo::value<bool>()->default_value(false))
    ("floodfill,f", bpo::value<bool>()->default_value(false))
    ("bandwidth,b", bpo::value<std::string>()->default_value("L"))
    ("enable-ssu", bpo::value<bool>()->default_value(true))
    ("enable-ntcp", bpo::value<bool>()->default_value(true))
    ("reseed-from,r", bpo::value<std::string>()->default_value(""))
    ("reseed-skip-ssl-check", bpo::value<bool>()->default_value(false));

  bpo::options_description client("\nclient");
  client.add_options()
    ("httpproxyport", bpo::value<int>()->default_value(4446))
    ("httpproxyaddress", bpo::value<std::string>()->default_value("127.0.0.1"))
    ("socksproxyport", bpo::value<int>()->default_value(4447))
    ("socksproxyaddress", bpo::value<std::string>()->default_value("127.0.0.1"))
    ("proxykeys", bpo::value<std::string>()->default_value(""))
    ("i2pcontrolport", bpo::value<int>()->default_value(0))
    ("i2pcontroladdress", bpo::value<std::string>()->default_value("127.0.0.1"))
    ("i2pcontrolpassword", bpo::value<std::string>()->default_value("itoopie"));
    //("reseed-to", bpo::value<std::string>()->default_value(""),
    // "Creates a reseed file for you to share\n"
    // "Example: ~/path/to/new/i2pseeds.su3\n")
  // Available command-line options
  bpo::options_description cli_options;
  cli_options
    .add(help)
    .add(system)
    .add(network)
    .add(client);
  // Available config file options
  bpo::options_description config_options;
  config_options
    .add(system)
    .add(network)
    .add(client);
  // Map and store command-line options
  bpo::store(
      bpo::command_line_parser(m_Args).options(cli_options).run(),
      m_KovriConfig);
  bpo::notify(m_KovriConfig);
  // Help options
  if (m_KovriConfig.count("help")) {
    std::cout << config_options << std::endl;
    throw std::runtime_error("for more details, see user-guide or config file");
  }
  // Parse config file after mapping command-line
  // TODO(anonimal): we want to be able to reload config file without original
  // cli args overwriting any *new* config file options
  SetupGlobalPath();
  std::string kovri_config = GetConfigFile().string();
  ParseKovriConfigFile(kovri_config, config_options, m_KovriConfig);
}

// TODO(unassigned): improve this function and use-case
void Configuration::ParseKovriConfigFile(
    std::string& file,
    bpo::options_description& options,
    bpo::variables_map& var_map) {
  std::ifstream filename(file.c_str());
  if (!filename)
    throw std::runtime_error("Could not open " + file + "!\n");
  bpo::store(bpo::parse_config_file(filename, options), var_map);
  bpo::notify(var_map);
}

void Configuration::SetupGlobalPath()
{
  context.SetCustomDataDir(
      m_KovriConfig["data-dir"].defaulted()
          ? kovri::core::GetDefaultDataPath().string()
          : m_KovriConfig["data-dir"].as<std::string>());
}

void Configuration::SetupLogging() {
  namespace logging = boost::log;
  namespace expr = boost::log::expressions;
  namespace sinks = boost::log::sinks;
  namespace attrs = boost::log::attributes;
  namespace keywords = boost::log::keywords;
  // Get global logger
  // TODO(unassigned): depends on global logging initialization. See notes in log impl
  auto core = logging::core::get();
  // Add core attributes
  core->add_global_attribute("TimeStamp", attrs::utc_clock());
  core->add_global_attribute("ThreadID", attrs::current_thread_id());
  // Get/Set filter log level
  auto log_level = m_KovriConfig["log-level"].as<std::uint16_t>();
  logging::trivial::severity_level severity;
  switch (log_level) {
    case 0:
        severity = logging::trivial::fatal;
      break;
    case 1:
        severity = logging::trivial::error;
      break;
    case 2:
        severity = logging::trivial::warning;
      break;
    case 3:
        severity = logging::trivial::info;
      break;
    case 4:
        severity = logging::trivial::debug;
      break;
    case 5:
        severity = logging::trivial::trace;
      break;
    default:
      throw std::invalid_argument("Configuration: invalid log-level, see documentation");
      break;
  };
  core->set_filter(expr::attr<logging::trivial::severity_level>("Severity") >= severity);
  // Create text backend + sink
  typedef sinks::synchronous_sink<sinks::text_ostream_backend> text_ostream_sink;
  auto text_sink = boost::make_shared<text_ostream_sink>();
  text_sink->locked_backend()->add_stream(
      boost::shared_ptr<std::ostream>(&std::clog, boost::null_deleter()));
  // Create file backend
  typedef sinks::asynchronous_sink<sinks::text_file_backend> text_file_sink;
  auto file_backend = boost::make_shared<sinks::text_file_backend>(
      keywords::file_name =
          m_KovriConfig["log-file-name"].defaulted()
              ? ((kovri::core::GetLogsPath() / "kovri_%Y-%m-%d.log").string())
              : m_KovriConfig["log-file-name"].as<std::string>(),
      keywords::time_based_rotation =
          sinks::file::rotation_at_time_point(0, 0, 0));  // Rotate at midnight
  // If debug/trace, enable auto flush to (try to) catch records right before segfault
  if (severity <= logging::trivial::debug)  // Our severity levels are processed in reverse
    file_backend->auto_flush();
  // Create file sink
  auto file_sink =
    boost::shared_ptr<text_file_sink>(std::make_unique<text_file_sink>(file_backend));
  // Set sink formatting
  logging::formatter format = expr::stream
      << "[" << expr::format_date_time(
          expr::attr<boost::posix_time::ptime>("TimeStamp"), "%Y.%m.%d %T.%f") << "]"
      << " [" << expr::attr<attrs::current_thread_id::value_type>("ThreadID") << "]"
      << " [" << logging::trivial::severity << "]"
      << "  " << expr::smessage;
  text_sink->set_formatter(format);
  file_sink->set_formatter(format);
  // Add sinks
  core->add_sink(text_sink);
  core->add_sink(file_sink);
  // Remove sinks if needed (we must first have added sinks to remove)
  bool log_to_console = m_KovriConfig["log-to-console"].as<bool>();
  bool log_to_file = m_KovriConfig["log-to-file"].as<bool>();
  if (!log_to_console)
    core->remove_sink(text_sink);
  if (!log_to_file)
    core->remove_sink(file_sink);
}

void Configuration::SetupAESNI() {
  // TODO(anonimal): implement user-option to disable AES-NI auto-detection
  kovri::core::SetupAESNI();
}

void Configuration::ParseTunnelsConfig() {
  auto file = GetTunnelsConfigFile().string();
  boost::property_tree::ptree pt;
  // Read file
  try {
    boost::property_tree::read_ini(file, pt);
  } catch (const std::exception& ex) {
    throw std::runtime_error(
        "Configuration: can't read " + file + ": " + ex.what());
    return;
  } catch (...) {
    throw std::runtime_error(
        "Configuration: can't read " + file + ": unknown exception");
    return;
  }
  // Parse on a per-section basis, store in tunnels config vector
  for (auto& section : pt) {
    kovri::client::TunnelAttributes tunnel{};
    try {
      // Get tunnel name and container for remaining attributes
      tunnel.name = section.first;
      const auto& value = section.second;
      // Get remaining attributes
      tunnel.type = value.get<std::string>(GetAttribute(Key::Type));
      tunnel.address = value.get<std::string>(GetAttribute(Key::Address), "127.0.0.1");
      tunnel.port = value.get<std::uint16_t>(GetAttribute(Key::Port));
      // Test which type of tunnel (client or server), add unique attributes
      if (tunnel.type == GetAttribute(Key::Client)
          || tunnel.type == GetAttribute(Key::IRC)) {
        tunnel.dest = value.get<std::string>(GetAttribute(Key::Dest));
        tunnel.dest_port = value.get<std::uint16_t>(GetAttribute(Key::DestPort), 0);
        tunnel.keys = value.get<std::string>(GetAttribute(Key::Keys), "");
        // Parse for CSV destinations + dest:port, then set appropriately
        ParseClientDestination(&tunnel);
      } else if (tunnel.type == GetAttribute(Key::Server)
                || tunnel.type == GetAttribute(Key::HTTP)) {
        tunnel.in_port = value.get<std::uint16_t>(GetAttribute(Key::InPort), 0);
        tunnel.keys = value.get<std::string>(GetAttribute(Key::Keys));  // persistent private key
        // Test/Get/Set for ACL
        auto white = value.get<std::string>(GetAttribute(Key::Whitelist), "");
        auto black = value.get<std::string>(GetAttribute(Key::Blacklist), "");
        // Ignore blacklist if whitelist is given
        if (!white.empty()) {
          tunnel.acl.list = white;
          tunnel.acl.is_white = true;
        } else if (!black.empty()) {
          tunnel.acl.list = black;
          tunnel.acl.is_black = true;
        }
      } else {
        throw std::runtime_error(
            "Configuration: unknown tunnel type="
            + tunnel.type + " of " + tunnel.name + " in " + file);
      }
    } catch (const std::exception& ex) {
      throw std::runtime_error(
          "Configuration: can't read tunnel "
          + tunnel.name + " params: " + ex.what());
    } catch (...) {
      throw std::runtime_error(
          "Configuration: can't read tunnel "
          + tunnel.name + " unknown exception");
    }
    // Save section for later client insertion
    m_TunnelsConfig.push_back(tunnel);
  }
}

const std::string Configuration::GetAttribute(Key key) {
  switch (key) {
    // Section types
    case Key::Type:
      return "type";
      break;
    case Key::Client:
      return "client";
      break;
    case Key::IRC:
      return "irc";
      break;
    case Key::Server:
      return "server";
      break;
    case Key::HTTP:
      return "http";
      break;
    // Client-tunnel specific
    case Key::Dest:
      return "dest";
      break;
    case Key::DestPort:
      return "dest_port";
      break;
    // Server-tunnel specific
    case Key::InPort:
      return "in_port";
      break;
    case Key::Whitelist:
      return "white_list";
      break;
    case Key::Blacklist:
      return "black_list";
      break;
    // Tunnel-agnostic
    case Key::Address:
      return "address";
      break;
    case Key::Port:
      return "port";
      break;
    case Key::Keys:
      return "keys";
      break;
    default:
      return "";  // not needed (avoids nagging -Wreturn-type)
      break;
  };
}

}  // namespace app
}  // namespace kovri
