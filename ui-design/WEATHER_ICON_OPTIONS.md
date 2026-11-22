# Weather icon style options

The three previews keep the current green-and-yellow screen layout and show the same weather samples (partly cloudy, rain, and sun) plus the four life-guide categories.

| Option | Weather family | Life-guide family | Visual direction |
| --- | --- | --- | --- |
| A | [Meteocons Line](https://github.com/basmilius/meteocons) | [Phosphor Bold](https://phosphoricons.com/) | Rounded, high-contrast outlines with more weather detail |
| B | [Meteocons Flat](https://github.com/basmilius/meteocons) | [Phosphor Duotone](https://phosphoricons.com/) | Two-tone filled shapes |
| C | [Material Symbols Rounded](https://fonts.google.com/icons) | [Material Symbols Rounded](https://fonts.google.com/icons) | Unified, rounded filled system icons |

Meteocons and Phosphor are MIT licensed. Material Symbols are Apache 2.0 licensed. The preview artwork is simplified to judge how each family reads at 24–48 px on the 320 × 240 display; it is not yet the production SVG-to-RGB565 conversion. After a choice, the selected source icons will be adapted to the display and mapped to the project's weather codes.

Preview generator: `Generate-WeatherIconOptions.ps1`.
