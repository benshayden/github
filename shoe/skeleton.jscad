/*jslint sloppy: true, vars: true, indent: 2 */
var PARAMS = [{'name': 'resolution', 'caption': 'Resolution:', 'type': 'int', 'default': 24}];
function curve(center, radius, start, end, maxedge) {
  var d = 2 * Math.asin(maxedge / (2 * radius)) * Math.sign(end - start);
  var p = [];
  while ((d > 0) ? (start < end) : (end < start)) {
    p.push([center[0] + (radius * Math.cos(start)), center[1] + (radius * Math.sin(start))]);
    start += d;
  }
  p.push([center[0] + (radius * Math.cos(end)), center[1] + (radius * Math.sin(end))]);
  return p;
}
var TAU = 2 * Math.PI;
function main() {
  var hf = 4;
  var hb = 5;
  var wf = 6;
  var wb = 7;
  var bs = 8;
  var cush = [3, 1, 1.5];
  var chr = 2;
  var lead = cube({size: [0.5, bs, 2]}).expand(0.5, 8);
  lead = union(lead, lead.translate([0, 0, bs - 2]));
  var cushion = cube({size: cush});
  cushion = union(cushion, cushion.translate([0, 0, bs - cush[2]]));
  var p = [
    [0, 0],
    [bs + hf + bs, 0],
    [bs + hf + bs, bs + wf + bs + wb + bs],
    [hf - hb, bs + wf + bs + wb + bs]
  ];
  var pl = p[p.length - 1];
  p = p.concat(curve([pl[0], pl[1] + chr], chr, 0.9 * TAU / 4, 2.95 * TAU / 4, 0.5).reverse(),
               curve([pl[0], pl[1] + chr + (bs / 2)], (bs - (2 * chr)) / 2, 3.1 * TAU / 4, 5 * TAU / 4, 0.5),
               curve(pl, bs, 1.1 * TAU / 4, 2.95 * TAU / 4, 1));
  p = p.concat([
    [hf - hb, bs + wf + bs + wb],
    [hf + bs, bs + wf + bs + wb],
    [hf + bs, bs],
    [0, bs]
  ]);
  p = p.concat(curve([0, 0], bs, 1.1 * TAU / 4, 3 * TAU / 4, 1),
               curve([0, -chr - (bs / 2)], (bs - (2 * chr)) / 2, 3.1 * TAU / 4, 5 * TAU / 4, 0.5),
               curve([0, -chr], chr, 0.9 * TAU / 4, 3.1 * TAU / 4, 0.5).reverse());
  return difference(
    union(
      linear_extrude({height: bs}, polygon(p)),
      cushion.translate([0, bs, 0]),
      cushion.translate([hf - hb, bs - cush[1] + wf + bs + wb, 0]),
      cushion.rotateZ(90).translate([bs + hf, bs + wf, 0])),
    lead.translate([bs / 2, 0, 0]),
    lead.translate([bs / 2 + hf - hb, bs + wf + bs + wb, 0]),
    lead.rotateZ(90).translate([bs + hf + bs, bs + wf + (bs / 2), 0]));
}
