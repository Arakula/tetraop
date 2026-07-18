========== TetraOP ==========
Copyright (C) 2026 Tilr

MacOS builds are unsigned, please let me know of any issues by opening a ticket.
Because the builds are unsigned you may have to run the following commands:

sudo xattr -dr com.apple.quarantine /path/to/your/plugin/TetraOP.component
sudo xattr -dr com.apple.quarantine /path/to/your/plugin/TetraOP.vst3
sudo xattr -dr com.apple.quarantine /path/to/your/plugin/TetraOP.lv2
sudo xattr -dr com.apple.quarantine /path/to/your/plugin/TetraOP.clap

The command above will recursively remove the quarantine flag from the plug-ins.

To install TetraOP copy the following files:

  * TetraOP.vst3 -> ~/Library/Audio/Plug-Ins/VST3/
  * TetraOp.clap -> ~/Library/Audio/Plug-Ins/CLAP/
  * TetraOP.component -> ~/Library/Audio/Plug-Ins/Components
  * presets -> ~/Library/Application Support/TetraOP
  * wavetables -> ~/Library/Application Support/TetraOP

The folders should be copied with the folder name included so the resulting folders should be:

~/Library/Audio/Plug-Ins/VST3/TetraOP.vst3
~/Library/Audio/Plug-Ins/CLAP/TetraOP.clap
~/Library/Audio/Plug-Ins/Components/TetraOP.component
~/Library/Application Support/TetraOP/presets/Factory
~/Library/Application Support/TetraOP/wavetables

Note that TetraOP folder will not exist until TetraOP runs for the first time.
Either run the plug-in a first time and then move the contents or create the TetraOP directory first.