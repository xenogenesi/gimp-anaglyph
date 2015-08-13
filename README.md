# gimp-anaglyph
gimp plugin to create and interactively edit red/cyan green/magenta anaglyphs


gimp-anaglyph
============

This is an interactive plugin to generate and edit red/cyan and green/magenta anaglyph with **gimp**.

**Still a work in progress**, it works but don't expect production quality, for instance, it still doesn't handle pixels vacated by displacement, it doesn't implemente color/half color or any optimized methods.

 **it just shift the red or the green component channel** by an amount specified with: the **maximum displacement** value in the GUI spin button **multiplied** by the **depth level** in the *depth map channel* which is **divided** by **255** before and finally **multiplied** with the **curve level** edited with the GUI panel.

`displacement = max * ( depth / 255 ) * curve factor`

**How it work**:

1. open an image and select the background layer
2. menu Filters > Map > Anaglyph...
    - the plugin will create one *output* layer and one *depth* channel and will make the depth channel active
    - the GUI panel should remain opened and above other windows
    - you can *edit* the source layer and the depth channel in any way you like
    - just move the mouse over the *do it* label at the bottom of the GUI panel to update the output

**Note**: once the GUI show up and the channel and layer are created, the source the depth and the output are **bounded** with the current plugin instance, to create a new anaglyph with a different image the plugin must be closed and re-opened.

**How to build**:

- checkout gimp repository
- install *cmake* and *libgimp2.0-dev* (that's in Debian, your distro may use different name it must contain the `gimp-2.0` include directory and the `gimptool-2.0` command)
- create a build dir, enter it, run `cmake` with the path to the checkout then `make` or `make xinstall`

the *xinstall* target will run `gimptool-2.0 --install-bin` which should copy the built plugin into your home gimp's plug-ins directory, `make` without *xinstall* will only build the plugin locally into the build directory therefore must be installed manually.

    mkdir build/ && cd build && cmake [path of the checkout]
    make [xinstall]

**TODO**:
- handle vacated pixels
- tile cache size editable within GUI panel (currently a size to cover the source layer, the depth channel and the output layer (+ the shadow) is allocated, **bigger the image more the memory used**)
- look to half and optimized implementations
- replace deprecated gtk2 widgets (like curve with a custom cairo implementation)
- see if gimp plugin API allow to check and refresh dirty tiles (without user interaction with the GUI panel)
- based on this plugin create a new one (not anaglyph) combining a source and a gradient with a *depth* channel (like Filters > Map > Gradient does, useful to fake thermal pictures)

