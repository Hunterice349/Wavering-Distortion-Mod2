Wavering Distortion Mod2 — Distortion DSP implementation package

Replace these files on your feature branch:
- src/DistortionDSP.h
- src/DistortionDSP.cpp
- src/PluginProcessor.cpp
- tests/DSPTests.cpp

Then apply EDITOR_CHANGES.txt to the existing PluginEditor.h and PluginEditor.cpp.
Do not replace PluginProcessor.h, CMakeLists.txt, WaveguideDSP files, or the build workflow.

Added:
- Five distortion algorithms: Drive, Overdrive, Distortion, Fuzz, Hard Clip.
- New APVTS parameter ID: distortionMode (default Drive / index 0).
- Existing parameter IDs and ranges are unchanged.
- Drive keeps the previous tanh transfer curve.
- Mode changes are smoothed/interpolated.
- Dry bypass and Mix=0 preserve finite input exactly, including values above +1 and below -1.
- Non-finite input is sanitized to zero.
- Wet path is bounded; dry path is not clipped.
- Expanded standalone DSP tests.

Local validation performed:
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic src/DistortionDSP.cpp tests/DSPTests.cpp
Result: All DistortionDSP tests passed.

This is a DSP-level local test only. The full JUCE/VST3 multi-platform build still needs to run in GitHub Actions after the files are committed.
