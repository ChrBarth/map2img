# map2img

A tool to convert DOOM(2)-Maps into SVG-images.

## example:

```
map2img -f DOOM.WAD -m E1M1 -o E1M1.svg -t -p 2 -s 0.2
```
creates an svg-file "E1M1.svg" from DOOM's first map (including things), scales it to 0.2 times the size

## Arguments:

```
-v (type: bool): verbose output (optional)
-f (type: string): WAD file (required)
-m (type: string): map name (e.g. E1M1) (optional)
-o (type: string): output file name (optional)
-l (type: bool): lists all maps in the wad file and exits (optional)
-t (type: bool): draw things (optional)
-s (type: float): scale factor (default: 0.5) (optional)
-p (type: integer): additional padding from the image borders (default: 0) (optional)
```

## TODO:

* make coloring customizable
* display sectors (and make them one shape instead of having a single line object for each linedef)
