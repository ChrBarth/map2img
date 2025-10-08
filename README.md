#map2img

A tool to convert DOOM(2)-Maps into SVG-images.

## usage:

```
map2img
```

## TODO:

* scale the svg to a sensible size
* commandline options to select WAD and mapname
* color linedefs depending on their special value (right now only two-sided linedefs are rendered grey instead of black), e.g. secrets, exits, switches,...
* make coloring customizable
* display players, monsters, items,... (maybe mark them with a simple circle)
* display sectors (and make them one shape instead of having a single line object for each linedef)
