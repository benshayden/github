/*jslint sloppy: true, vars: true, indent: 2 */
var PARAMS = [{'name': 'resolution', 'caption': 'Resolution:', 'type': 'int', 'default': 24}];
function circle(center, radius, start, end, maxedge) {
  var d = Math.asin(maxedge / (2 * radius)) * Math.sign(end - start);
  var p = [];
  while ((d > 0) ? (start < end) : (end < start)) {
    p.push([]);
  }
  return p;
}
function main() {
  var hf = 4;
  var hb = 5;
  var wf = 6;
  var wb = 7;
  var bs = 8;
  var cush = [3, 1, 1.5];
  var lead = cube({size: [0.5, bs, 2]}).expand(0.5, 8);
  lead = union(lead, lead.translate([0, 0, bs - 2]));
  var cushion = cube({size: cush});
  cushion = union(cushion, cushion.translate([0, 0, bs - cush[2]]));
  var p = [
    [0, 0],
    [bs + hf + bs, 0],
    [bs + hf + bs, bs + wf + bs + wb + bs],
    [hf - hb, bs + wf + bs + wb + bs],
    [hf - hb, bs + wf + bs + wb],
    [hf + bs, bs + wf + bs + wb],
    [hf + bs, bs],
    [0, bs]
  ];
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
