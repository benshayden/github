Flat keyboards require fingers to reach a long way. 3D Printing allows custom tailoring a keyboard to reduce travel distance to zero by placing all of the buttons literally right at your fingertips.

[](TODO picture)

It's easy to measure fingertips, but more difficult to know in advance the best way to position the fingers relative to each other. A stiff but malleable wire such as a coat hanger can keep the buttons in place while typing, and still allow remodelling without waiting for a 3d-printer to reprint the entire keyboard and waste material.

[](TODO picture)

Materials:
 * [3D-printed parts](http://openjscad.org/#https://raw.githubusercontent.com/benshayden/github/master/keyboard/keyboard.jscad). Configure the parameters to fit your fingers. When it looks right, select each part and download the STL files for each part. [Clean up the STL files](https://netfabb.azurewebsites.net/). Print them at [fictiv](https://www.fictiv.com/) or [Shapeways](http://www.shapeways.com/) or [i.materialise](http://i.materialise.com/) or [UPS](http://www.theupsstore.com/small-business-solutions/Pages/3d-printing-locations.aspx) or a [local hackerspace](http://hackerspaces.org/wiki/List_of_Hacker_Spaces) or your own printer.
 * [multi-purpose 12-14AWG galvanized steel wire](http://www.amazon.com/Hillman-Galvanized-Wire-Multi-Purpose-Galv/dp/B002YGMLVO)
 * Arduino-compatible USB Serial-capable microcontroller e.g. [Teensy-LC](https://www.pjrc.com/store/teensylc.html)
 * [buttons](http://www.digikey.com/product-detail/en/EVQ-QJJ05Q/P8029SCT-ND/165317) -- If you use other switches such as [Cherry MX](http://www.digikey.com/product-search/en?keywords=MX1A-C1NN), then you can still use the 3d-printed triangular base, but you'll need to redesign the button mounts, or use [thermoplastic](http://www.adafruit.com/products/2504).
 * [diodes](http://www.digikey.com/product-detail/en/1N914BTR/1N914BCT-ND/458919)
 * current-limiting resistors
 * jumper wire
 * soldering iron, solder
 * optional [optical motion sensor](https://www.tindie.com/products/jkicklighter/adns-9800-optical-laser-sensor/)

Use [keymaps.html](https://cdn.rawgit.com/benshayden/github/c483f8/keyboard/keymaps.html) to generate the KEYMAPS. Copy-paste them into keyboard.ino before programming the microcontroller.

If you use Linux, run `python heatmap.py ~/.keyboardheatmap` as a daemon for a few weeks to generate a histogram of the keys that you use most frequently. Copy-paste the generated .keyboardheatmap into keymaps.html to generate KEYMAPS so that keys that you use more frequently are mapped to buttons that are easier for you to press.
