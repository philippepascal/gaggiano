# SD card content for the display

Format the card as FAT and copy the `gaggia/` folder to its root. The firmware
(`gaggiano-v2/gaggia_config.cpp`) expects:

| Path | Purpose |
|---|---|
| `/gaggia/frank.bmp` | boot splash, 800x480 BMP. On first boot the firmware writes a faster `frank_opt.bin` next to it |
| `/gaggia/gaggia_settings.csv` | created by the firmware on first run |
| `/gaggia/profiles/` | brew profiles, one CSV per profile, created from the UI |
| `/gaggia/selectedProfile` | name of the active profile |
| `/gaggia/gaggia_logs.csv` | controller log, recreated at boot when logging is on |

Only `frank.bmp` needs to be present; everything else is generated.
