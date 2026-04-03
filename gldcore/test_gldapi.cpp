#include "gldapi.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <cmath>
#include <nlohmann/json.hpp>
#include "timestamp.h"

// ─── Helpers ─────────────────────────────────────────────────────

std::string get_base_filename(const std::string& filepath) {
    size_t pos = filepath.find_last_of("/\\");
    std::string filename = (pos == std::string::npos) ? filepath : filepath.substr(pos + 1);
    size_t dot = filename.find_last_of('.');
    if (dot != std::string::npos) {
        filename = filename.substr(0, dot);
    }
    return filename;
}

nlohmann::json read_checkpoint_file(const std::string& filepath) {
    std::ifstream ifs(filepath);
    if (!ifs.is_open()) {
        throw std::runtime_error("Could not open checkpoint file: " + filepath);
    }
    nlohmann::json j;
    ifs >> j;
    return j;
}

// ─── JSON comparison ─────────────────────────────────────────────

struct DiffEntry {
    std::string path;
    std::string value_a;
    std::string value_b;
};

static const std::vector<std::string> SKIP_KEYS = {"__preamble"};

// Exact paths (rooted at "$") that are non-deterministic and should be ignored
static const std::vector<std::string> SKIP_PATHS = {
    "$.clock.starttime",
    "$.globals.checkpoint_loaded",
    "$.globals.randomseed",
};    


// Returns true when a comparison path should be skipped
static bool should_skip_path(const std::string& path) {
    for (const auto& p : SKIP_PATHS) {
        if (path == p) return true;
    }
    // Skip any field named "rng_state" regardless of nesting depth
    const std::string suffix = ".rng_state";
    if (path.size() >= suffix.size() &&
        path.compare(path.size() - suffix.size(), suffix.size(), suffix) == 0)
        return true;
    return false;
}

void compare_json(const nlohmann::json& a, const nlohmann::json& b,
                  const std::string& path, std::vector<DiffEntry>& diffs,
                  double tol) {
    if (should_skip_path(path)) return;
    if (a.type() != b.type()) {
        diffs.push_back({path, a.dump(), b.dump()});
        return;
    }
    if (a.is_object()) {
        for (auto it = a.begin(); it != a.end(); ++it) {
            std::string child = path + "." + it.key();
            if (b.contains(it.key())) {
                compare_json(it.value(), b[it.key()], child, diffs, tol);
            } else {
                diffs.push_back({child, it.value().dump(), "<missing>"});
            }
        }
        for (auto it = b.begin(); it != b.end(); ++it) {
            if (!a.contains(it.key())) {
                diffs.push_back({path + "." + it.key(), "<missing>", it.value().dump()});
            }
        }
    } else if (a.is_array()) {
        size_t max_sz = std::max(a.size(), b.size());
        for (size_t i = 0; i < max_sz; i++) {
            std::string child = path + "[" + std::to_string(i) + "]";
            if (i >= a.size())      diffs.push_back({child, "<missing>", b[i].dump()});
            else if (i >= b.size()) diffs.push_back({child, a[i].dump(), "<missing>"});
            else                    compare_json(a[i], b[i], child, diffs, tol);
        }
    } else if (a.is_number_float()) {
        double va = a.get<double>();
        double vb = b.get<double>();
        if (std::abs(va - vb) > tol) {
            diffs.push_back({path, std::to_string(va), std::to_string(vb)});
        }
    } else {
        if (a != b) {
            diffs.push_back({path, a.dump(), b.dump()});
        }
    }
}

// ─── Main ────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <model.glm>"
                  << " [--checkpoint] [--restore] [--compare]"
                  << " [--steps N] [--tolerance T] [--output FILE]"
                  << std::endl;
        return 1;
    }

    std::string fileName = argv[1];
    std::string base = get_base_filename(fileName);

    // Parse flags
    bool checkpoint_mode = false;
    bool restore_mode    = false;
    bool compare_mode    = false;
    int  num_steps       = 2;
    double tolerance     = 0;
    std::string output_file;

    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--checkpoint")        checkpoint_mode = true;
        else if (arg == "--restore")      restore_mode = true;
        else if (arg == "--compare")      compare_mode = true;
        else if (arg == "--steps"     && i + 1 < argc) num_steps = std::stoi(argv[++i]);
        else if (arg == "--tolerance" && i + 1 < argc) tolerance = std::stod(argv[++i]);
        else if (arg == "--output"    && i + 1 < argc) output_file = argv[++i];
    }

    // ══════════════════════════════════════════════════════════════
    // MODE: --compare  (no GLD init — just compare two JSON files)
    // ══════════════════════════════════════════════════════════════
    if (compare_mode) {
        std::string full_ckpt     = base + "_full_checkpoint.json";
        std::string restored_ckpt = base + "_restored_checkpoint.json";

        printf("Comparing checkpoints:\n");
        printf("  Baseline : %s\n", full_ckpt.c_str());
        printf("  Restored : %s\n", restored_ckpt.c_str());
        printf("  Tolerance: %e\n", tolerance);

        nlohmann::json a, b;
        try {
            a = read_checkpoint_file(full_ckpt);
            b = read_checkpoint_file(restored_ckpt);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }

        // Strip non-deterministic keys
        for (const auto& key : SKIP_KEYS) { a.erase(key); b.erase(key); }

        std::vector<DiffEntry> diffs;
        compare_json(a, b, "$", diffs, tolerance);

        if (diffs.empty()) {
            printf("\n  TEST PASSED: Checkpoints match.\n\n");
            return 0;
        } else {
            printf("\n  TEST FAILED: %zu difference(s):\n\n", diffs.size());
            printf("  %-60s | %-30s | %-30s\n", "Path", "Full Run", "Restored");
            printf("  %s\n", std::string(125, '-').c_str());
            for (const auto& d : diffs) {
                printf("  %-60s | %-30s | %-30s\n",
                       d.path.c_str(), d.value_a.c_str(), d.value_b.c_str());
            }
            return 1;
        }
    }

    // ══════════════════════════════════════════════════════════════
    // GLD modes (one GLD instance per process)
    // ══════════════════════════════════════════════════════════════

    GridLabD gld;

    // Determine what file to load
    std::string loadFile = fileName;
    if (restore_mode) {
        loadFile = base + "_partial_checkpoint.json";
        printf("Restoring from: %s\n", loadFile.c_str());
    }

    // Resolve output filename
    if (output_file.empty()) {
        if (checkpoint_mode)    output_file = base + "_partial_checkpoint.json";
        else if (restore_mode)  output_file = base + "_restored_checkpoint.json";
        else                    output_file = base + "_full_checkpoint.json";
    }

    // Build argv for load_glm
    std::vector<const char*> args = {loadFile.c_str(), "--verbose"};
    int test_argc = static_cast<int>(args.size());
    std::vector<char*> test_argv;
    for (auto& a : args) test_argv.push_back(const_cast<char*>(a));

    try {
        gld.load_glm(test_argc, test_argv.data());
    } catch (const std::exception& e) {
        std::cerr << "Error loading: " << e.what() << std::endl;
        return 1;
    }

    if (checkpoint_mode) {
        double sim_time;
        for (int i = 0; i < num_steps; i++) {
            gld.step(sim_time);
        }
        printf("Completed %d steps. Simulation time: %.2f\n", num_steps, sim_time);

        // Pass the filename so do_checkpoint writes to the right place
        nlohmann::json checkpoint = gld.get_checkpoint_json(output_file.c_str());
        printf("Checkpoint saved to: %s\n", output_file.c_str());
    }
    else if (restore_mode) {
        if (!gld.gld_model.is_null()) {
            std::cout << gld.gld_model.dump(4) << std::endl;
        }
        gld.run();

        nlohmann::json checkpoint = gld.get_checkpoint_json(output_file.c_str());
        printf("Restored checkpoint saved to: %s\n", output_file.c_str());
    }
    else {
        gld.run();

        nlohmann::json checkpoint = gld.get_checkpoint_json(output_file.c_str());
        printf("Full checkpoint saved to: %s\n", output_file.c_str());
    }

    gld.exit_gld(loadFile.c_str());
    return 0;
}