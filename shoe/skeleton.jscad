function getParameterDefinitions() {
  return [
    {name: 'hf', caption: 'Front height mm:', type: 'float', initial: 7},
    {name: 'hb', caption: 'Back height mm:', type: 'float', initial: 9},
    {name: 'wf', caption: 'Front width mm:', type: 'float', initial: 8},
    {name: 'wb', caption: 'Back width mm:', type: 'float', initial: 10},
    {name: 'chr', caption: 'Coat hanger radius mm:', type: 'float', initial: 1.5875}];
}
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
function main(params) {
  var bs = 8;
  var cush = [3, 1, 1.5];
  // TODO Fix the chr/or math
  var or = 1.4;
  var lead = linear_extrude({height: bs}, polygon([[0.5, 0], [0, 0.5], [0, 2.5], [0.5, 3], [1, 2.5], [1, 0.5]])).rotateX(90).translate([0, bs, -0.5]);
  lead = union(lead, lead.translate([0, 0, bs - 2]));
  var cushion = cube({size: cush});
  cushion = union(cushion, cushion.translate([0, 0, bs - cush[2]]));
  var p = [
    [0, 0],
    [bs + params.hf + bs, 0],
    [bs + params.hf + bs, bs + params.wf + bs + params.wb + bs],
    [params.hf - params.hb, bs + params.wf + bs + params.wb + bs]
  ];
  var pl = p[p.length - 1];
  p = p.concat(
    curve([pl[0] - 0.5, pl[1] + params.chr], params.chr, 2.95 * TAU / 4, 0.6 * TAU / 4, 0.5),
    curve([pl[0] + 0.8, pl[1] + (2 * params.chr) + 1], or, 3.1 * TAU / 4, 5 * TAU / 4, 0.5),
    curve([pl[0], pl[1] - ((params.chr + 1) / 2)], (bs / 2) + params.chr + 1, 1.1 * TAU / 4, 2.95 * TAU / 4, 1));
  p = p.concat([
    [params.hf - params.hb, bs + params.wf + bs + params.wb],
    [params.hf + bs, bs + params.wf + bs + params.wb],
    [params.hf + bs, bs],
    [0, bs]
  ]);
  p = p.concat(
    curve([0, (params.chr + 1) / 2], (bs / 2) + params.chr + 1, 1.1 * TAU / 4, 2.9 * TAU / 4, 1),
    curve([0.8, (-2 * params.chr) - 1], or, 3.1 * TAU / 4, 5.2 * TAU / 4, 0.5),
    curve([-0.5, -params.chr], params.chr, 3.4 * TAU / 4, 0.9 * TAU / 4, 0.5));
  p = linear_extrude({height: bs}, polygon(p));
  var cushions = union(cushion.translate([0, bs, 0]),
    cushion.translate([params.hf - params.hb, bs - cush[1] + params.wf + bs + params.wb, 0]),
    cushion.rotateZ(90).translate([bs + params.hf, bs + params.wf, 0]));
  //p = union(p, cushions);
  var leads = union(lead.translate([bs / 2, 0, 0]),
    lead.translate([bs / 2 + params.hf - params.hb, bs + params.wf + bs + params.wb, 0]),
    lead.rotateZ(90).translate([bs + params.hf + bs, bs + params.wf + (bs / 2), 0]));
  p = difference(p, leads);
  var button = cube({size: [bs, 3, bs]}).setColor([0, 0, 0]);
  button = union(button, cylinder({r: 2, h: 3, center: true}).setColor([0.5, 0.5, 0.5]).rotateX(90).translate([bs / 2, 4.5, (bs / 2)]));
  var buttons = union(
    button.translate([0, bs, 0]),
    button.rotateX(180).translate([params.hf - params.hb, bs + params.wf + bs + params.wb, bs]),
    button.rotateZ(90).translate([bs + params.hf, bs + params.wf, 0]));
  //p = union(p, buttons);
  return p;
}
