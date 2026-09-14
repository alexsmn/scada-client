#pragma once

#include <filesystem>
#include <string_view>

// Returns the directory where captured PNGs should land.
// The caller must provide `--out <dir>`.
std::filesystem::path GetOutputDir();

// Returns the output path for `filename` under the run's `--theme`.
//
// The gallery holds every capture under more than one appearance, in one flat
// directory: the run's DEFAULT theme keeps the bare name from
// `screenshot_data.json` (`devices.png`) and every other theme renders beside
// it with a suffix (`devices-light.png`). Without this the second run simply
// overwrote the first, because the output path was `GetOutputDir() /
// spec.filename` with nothing in it that varied.
//
// Dark is the unsuffixed one because dark is what this client actually starts
// in — `screenshot_options.h` defaults `theme` to it, and there has been no
// un-themed appearance since `SeverityTheme::kLegacy` went on 2026-08-31. The
// web gallery makes the opposite choice for the same reason: its default is
// light, so `web-x.png` is light and `web-x-dark.png` is dark. The rule is
// shared even though the suffixes differ — the bare name is whatever a fresh
// user of that client sees — and the two galleries' filenames were never
// meant to correspond anyway (root CLAUDE.md, "The two screenshot galleries
// join on the matrix row, not on filenames").
std::filesystem::path OutputPathFor(std::string_view filename);

// The naming rule itself, without the process-global options behind
// `OutputPathFor`. Split out so it can be tested: the options are initialised
// once from the real command line and have no setter, so a test that went
// through the wrapper could only ever observe the theme this process happens
// to have been started with.
std::filesystem::path ThemedOutputPath(const std::filesystem::path& dir,
                                       std::string_view filename,
                                       std::string_view theme);
