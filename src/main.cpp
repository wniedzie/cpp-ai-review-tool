#include <iostream>

#include "cli/cli_args.hpp"

int main(int argc, char* argv[]) {
    const auto args = cli::parse_args(argc, argv);
    if (!args) {
        std::cerr << "error: " << args.error() << '\n';
        return 2;
    }

    // TODO(FR-10): check and display one-time privacy warning
    // TODO: run analysis pipeline

    return 0;
}
