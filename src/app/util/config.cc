/**                                                                                           //
 * Copyright (c) 2015-2016, The Kovri I2P Router Project                                      //
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

#include "config.h"

#include <string>
#include <vector>

#include "core/util/log.h"
#include "crypto/rand.h"

namespace i2p {
namespace util {
namespace config {

namespace bpo = boost::program_options;

std::string kovri_config, tunnels_config;
bpo::variables_map var_map;

bool ParseArgs(
    int argc,
    char* argv[]) {
  // Random generated port if none is supplied via CLI or config
  // See: i2p.i2p/router/java/src/net/i2p/router/transport/udp/UDPEndpoint.java
  // TODO(unassigned): move this elsewhere (outside of ParseArgs()) when possible
  size_t port = i2p::crypto::RandInRange<size_t>(9111, 30777);
  // Map options values from CLI and config
  bpo::options_description help("help");
  help.add_options()
    ("help,h",

      "General usage:\n\n"

      "$ ./kovri\n\n"

      "A random port will be generated with each use.\n"
      "You can specify a port with the --port option\n"
      "or you can set one in the config file instead.\n\n"

      "Reload configuration file:\n\n"

      "$ pkill -HUP kovri\n\n"

      "Read kovri.conf and tunnels.conf for more options\n\n")

    ("help-with", bpo::value<std::string>(),

      "Help with a specific option.\n\n"

      "Available options:\n"
      "==================\n\n"

      "all | system | network | client\n\n"

      "Examples\n"
      "========\n\n"

      "List all options:\n\n"
      "$ ./kovri --help-with all\n\n"

      "List only system-related options:\n\n"
      "$ ./kovri --help-with system");

  bpo::options_description system("\nsystem");
  system.add_options()
    ("host", bpo::value<std::string>()->default_value("127.0.0.1"),
     "The external IP (deprecated).\n"
     "Default: external interface")

    ("port,p", bpo::value<int>()->default_value(port),
     "Port to listen on.\n"
     "Default: random (then saved to router.info)")

    ("daemon,d", bpo::value<bool>()->default_value(false),
     "Enable or disable daemon mode\n"
     "1 = enabled, 0 = disabled\n")

    ("service,s", bpo::value<std::string>()->default_value(""),
     "Windows only:\n"
     "./kovri --service install  # installs service\n"
     "./kovri --service remove   # removes service\n")

     ("log-to-console", bpo::value<bool>()->default_value(true),
     "Enable or disable console log output\n"
     "1 = enabled, 0 = disabled\n"
     "Default: 1 = enabled\n")

    ("log-to-file", bpo::value<bool>()->default_value(true),
     "Enable or disable logging to file\n"
     "1 = enabled, 0 = disabled\n"
     "Default: 0 = disabled\n")

    ("log-file-name", bpo::value<std::string>()->default_value(
        (i2p::util::filesystem::GetLogsPath() / "kovri_%1N.log").string()),
     "Sets log filename\n"
     "Default (with rotation): kovri_0.log, kovri_1.log, etc.\n")

    ("log-levels", bpo::value<std::vector<std::string>>()->
                   // Note: we set a default value during validation and
                   // leave blank here to prevent bad_any_cast exception.
                   default_value(std::vector<std::string>(), "")->multitoken(),
     "Log levels to report.\n"
     "Options: info warn error debug\n"
     "Examples:\n"
     "./kovri --log-levels info  # produces info only\n"
     "./kovri --log-levels warn error  # warn/error only\n"
     "Default: all levels [info warn error debug]\n")

    ("kovriconf,c", bpo::value<std::string>(&kovri_config)->default_value(
        i2p::util::filesystem::GetFullPath("kovri.conf")),
     "Options specified on the command line take"
     "precedence over those in the config file.\n")

    ("tunnelsconf,t", bpo::value<std::string>(&tunnels_config)->default_value(
        i2p::util::filesystem::GetFullPath("tunnels.conf")),
     "Tunnels Config file\n");

  bpo::options_description network("\nnetwork");
  network.add_options()
    ("v6,6", bpo::value<bool>()->default_value(false),
     "1 to enable IPv6\n"
     "1 = enabled, 0 = disabled\n")

    ("floodfill,f", bpo::value<bool>()->default_value(false),
     "1 to enable router router as floodfill\n"
     "1 = enabled, 0 = disabled\n")

    ("bandwidth,b", bpo::value<std::string>()->default_value("L"),
     "L if bandwidth is limited to 32Kbs/sec, O if not\n"
     "Always O if floodfill, otherwise L by default\n")

    ("reseed-from,r", bpo::value<std::string>()->default_value(""),
     "File or URL from which to reseed\n"
     "Examples:\n"
     "./kovri -r ~/local/path/to/i2pseeds.su3\n"
     "./kovri -r https://my.server.tld/i2pseeds.su3\n"
     "Note: if the server in your URL is not one of the "
     "hard-coded reseed servers, either use --reseed-skip-ssl-check"
     "or put your server's certificate in with the others. "
     "They are located in the .kovri data directory\n")

    ("reseed-skip-ssl-check", bpo::value<bool>()->default_value(false),
     "Skip SSL check for reseed host. Useful for custom reseed servers\n"
     "Examples:\n"
     "./kovri --reseed-skip-ssl-check -r https://my.server.tld/i2pseeds.su3\n");

  bpo::options_description client("\nclient");
  client.add_options()
    ("httpproxyport", bpo::value<int>()->default_value(4446),
     "The HTTP Proxy port to listen on\n")

    ("httpproxyaddress", bpo::value<std::string>()->default_value("127.0.0.1"),
     "The HTTP Proxy address to listen on\n")

    ("socksproxyport", bpo::value<int>()->default_value(4447),
     "The SOCKS Proxy port to listen on\n")

    ("socksproxyaddress", bpo::value<std::string>()->default_value("127.0.0.1"),
     "The SOCKS Proxy address to listen on\n")

    ("proxykeys", bpo::value<std::string>()->default_value(""),
     "Optional keys file for proxy's local destination\n")

    ("i2pcontrolport", bpo::value<int>()->default_value(0),
     "Port of I2P control service (usually 7650)\n"
     "I2PControl is disabled if not specified\n")

    ("i2pcontroladdress", bpo::value<std::string>()->default_value("127.0.0.1"),
     "Address of I2P control service\n"
     "Default: 127.0.0.1 (only used if I2PControl is enabled)\n")

    ("i2pcontrolpassword", bpo::value<std::string>()->default_value("itoopie"),
     "I2P control service password\n");

    //("reseed-to", bpo::value<std::string>()->default_value(""),
    // "Creates a reseed file for you to share\n"
    // "Example: ~/path/to/new/i2pseeds.su3\n")

  // Default visible option
  bpo::options_description kovri("");
  kovri.add(help);
  // Available config file options
  bpo::options_description config_options;
  config_options
    .add(system)
    .add(network)
    .add(client);
  // Available cli options
  bpo::options_description cli_options;
  cli_options
    .add(help)
    .add(system)
    .add(network)
    .add(client);
  // Map and store cli options
  bpo::store(bpo::parse_command_line(argc, argv, cli_options), var_map);
  bpo::notify(var_map);
  // Parse config after mapping cli
  ParseConfigFile(kovri_config, config_options, var_map);
  // Set logging options
  if (!SetLoggingOptions())
    return false;
  // Display --help and --help-with
  if (var_map.count("help")) {
    std::cout << kovri << std::endl;
    return false;
  }
  if (var_map.count("help-with")) {
    const std::string& help_option = var_map["help-with"].as<std::string>();
    if (help_option == "all") {
      std::cout << config_options;  // We don't need .add(help)
    } else if (help_option == "system") {
      std::cout << system;
    } else if (help_option == "network") {
      std::cout << network;
    } else if (help_option == "client") {
      std::cout << client;
    } else {
      std::cout << "Unknown option '" << help_option << "'"
      << "\nTry using --help" << std::endl;
    }
    return false;
  }
  return true;
}

void ParseConfigFile(
    std::string& config,
    bpo::options_description& config_options,
    bpo::variables_map& var_map) {
  std::ifstream ifs(config.c_str());
  if (!ifs) {
    std::cout << "Could not open " << config << "!\n";
  } else {
    bpo::store(
        bpo::parse_config_file(
            ifs,
            config_options),
        var_map);
    bpo::notify(var_map);
  }
}

bool SetLoggingOptions() {
  namespace log = i2p::util::log;
  /**
   * TODO(unassigned): write custom validator for log-levels
   * so we can set values via config file.
   */
  // Test for valid log-levels input
  auto arg_levels = var_map["log-levels"].as<std::vector<std::string>>();
  auto global_levels = log::GetGlobalLogLevels();
  if (arg_levels.size()) {
    if (arg_levels.size() > global_levels.size()) {
      std::cout << "Invalid number of log levels. Maximum allowed: "
                << global_levels.size() << std::endl;
      return false;
    }
    // Verify validity of log levels
    for (auto& level : arg_levels) {
      auto result = global_levels.find(level);
      if (result == global_levels.end()) {
        std::cout << "Invalid log-level(s). See help for options" << std::endl;
        return false;
      }
    }
  } else {
    // Set default log-levels if none present
    for (auto& level : global_levels)
      arg_levels.push_back(level.first);
  }
  // Set new global log-levels
  log::SetGlobalLogLevels(arg_levels);
  // Set other logging options
  log::SetOptionLogToConsole(var_map["log-to-console"].as<bool>());
  log::SetOptionLogToFile(var_map["log-to-file"].as<bool>());
  log::SetOptionLogFileName(var_map["log-file-name"].as<std::string>());
  return true;
}

}  // namespace config
}  // namespace util
}  // namespace i2p
