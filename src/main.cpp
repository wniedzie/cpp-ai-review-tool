#include <iostream>

#include "cli/cli_args.hpp"
#include "config/config.hpp"
#include "config/config_loader.hpp"

int main(int argc, char* argv[]) {
    const auto args = cli::parse_args(argc, argv);
    if (!args) {
        std::cerr << "error: " << args.error() << '\n';
        return 2;
    }

    const auto cfg = config::ConfigLoader::load(
        cli::to_config_args(*args), args->config_file);
    if (!cfg) {
        std::cerr << "error: " << config::to_string(cfg.error()) << '\n';
        return 2;
    }

    // TODO(FR-10): check and display one-time privacy warning
    // TODO: run analysis pipeline with cfg

    return 0;
}
