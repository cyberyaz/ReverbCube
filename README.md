Reverb Cube
Stereo reverb plugin built in JUCE. Parameters are controlled by dragging the
corners of an isometric cube — the cube's dimensions map directly to the
reverb settings. Uses JUCE's built-in juce::dsp::Reverb under the hood.

Controls
Width (X axis) — "Room" size
Height (Y axis) — Damping (taller = less damping)
Depth (Z axis) — Wet mix
Width slider — Stereo width (label strip at bottom)

Building
Open the .jucer file in Projucer, set your JUCE path, and export to your IDE
macOS: ~/Library/Audio/Plug-Ins/VST3/ or ~/Library/Audio/Plug-Ins/Components/
Windows: C:\Program Files\Common Files\VST3\
Rescan in your DAW after building
