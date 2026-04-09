#include <iostream>

#include "cli/cli_args.hpp"
#include "config/config.hpp"
#include "config/config_loader.hpp"
#include "engine/review_engine.hpp"
#include "llm/llm_error.hpp"

int main(int argc, char* argv[]) {
    const auto args = cli::parse_args(argc, argv);
    if (!args) {
        std::cerr << "error: " << args.error() << '\n';
        return 2;
    }

    const auto cfg = config::ConfigLoader::load(cli::to_config_args(*args), args->config_file);
    if (!cfg) {
        std::cerr << "error: " << config::to_string(cfg.error()) << '\n';
        return 2;
    }

    // TODO(FR-10): check and display one-time privacy warning

    auto engine = engine::make_review_engine(*cfg);
    if (!engine) {
        std::cerr << "error: " << llm::to_string(engine.error()) << '\n';
        return 2;
    }

    // TODO(file-collection): replace empty string with collected source content
    const auto result = engine->run("");
    if (!result) {
        std::cerr << "error: " << llm::to_string(result.error()) << '\n';
        return 2;
    }

    if (cfg->dry_run) {
        std::cout << "Dry run — estimated input tokens: " << result->input_tokens << '\n';
    } else {
        std::cout << result->content << '\n';
    }

    return 0;
}
