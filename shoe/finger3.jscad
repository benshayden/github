var show_buttons = false;
function getParameterDefinitions() {
  return [
    {name: 'chr', caption: 'Coat hanger radius mm:', type: 'float', initial: 1.4},
    {name: 'hf', caption: 'front height mm:', type: 'float', initial: 7},
    {name: 'hb', caption: 'back height mm:', type: 'float', initial: 9},
    {name: 'wf', caption: 'front width mm:', type: 'float', initial: 8},
    {name: 'wb', caption: 'back width mm:', type: 'float', initial: 10}];
}

var TAU = 2 * Math.PI;

function curve(center, radius, start, end, maxedge) {
  start *= TAU;
  end *= TAU;
  var d = 2 * Math.asin(maxedge / (2 * radius)) * Math.sign(end - start);
  var p = [];
  while ((d > 0) ? (start < end) : (end < start)) {
    p.push([center[0] + (radius * Math.cos(start)), center[1] + (radius * Math.sin(start))]);
    start += d;
  }
  p.push([center[0] + (radius * Math.cos(end)), center[1] + (radius * Math.sin(end))]);
  return p;
}

var cha = 4;  // coat hanger angle rad
var bs = 8;  // button length and width mm
var cush = [3, 1, 1.5];  // cushion dimensions mm
var or = 1;  // overhang radius mm

function button() {
  var b = cube({size: [bs, 3, bs]}).setColor([0, 0, 0]);
  b = union(b, cylinder({r: 2, h: 2, center: true}).setColor([0.5, 0.5, 0.5]).rotateX(90).translate([bs / 2, 4, (bs / 2)]));
  return b;
}
function lead() {
  var l = linear_extrude({height: bs}, polygon([[0.5, 0], [0, 0.5], [0, 2.5], [0.5, 3], [1, 2.5], [1, 0.5]])).rotateX(90).translate([0, bs, -0.5]);
  l = union(l, l.translate([0, 0, bs - 2]));
  return l;
}

function main(params) {
  var icha = (TAU - cha) / 2;
  var chc = [params.chr * Math.cos(icha), params.chr * Math.sin(icha)];
  var ohc = [or * Math.cos(icha), or * Math.sin(icha)];
  var ar = (bs / 2) + chc[1] + ohc[1]; // arch radius
  var l = lead();
  var leads = union(
    l.translate([bs / 2, 0, 0]),
    l.translate([bs / 2 + params.hf - params.hb, bs + params.wf + bs + params.wb, 0]),
    l.rotateZ(90).translate([bs + params.hf + bs, bs + params.wf + (bs / 2), 0]));
  var cushion = cube({size: cush});
  cushion = union(cushion, cushion.translate([0, 0, bs - cush[2]]));
  var cushions = union(
    cushion.translate([0, bs, 0]),
    cushion.translate([params.hf - params.hb, bs - cush[1] + params.wf + bs + params.wb, 0]),
    cushion.rotateZ(90).translate([bs + params.hf, bs + params.wf, 0]));
  var b = button();
  var buttons = union(
    b.translate([0, bs, 0]),
    b.rotateX(180).translate([params.hf - params.hb, bs + params.wf + bs + params.wb, bs]),
    b.rotateZ(90).translate([bs + params.hf, bs + params.wf, 0]));
  var p = [
    [0, 0],
    [bs + params.hf + bs, 0],
    [bs + params.hf + bs, bs + params.wf + bs + params.wb + bs],
    [params.hf - params.hb, bs + params.wf + bs + params.wb + bs]
  ];
  var pl = p[p.length - 1];
  p = p.concat(
    curve([pl[0] - chc[0], pl[1] + chc[1]], params.chr, 0.75, 0.25, 0.5),
    curve([pl[0] + ohc[0], pl[1] + (2 * chc[1]) + ohc[1]], or, 0.75, 1.25, 0.5),
    curve([pl[0], pl[1] - bs + ar], ar, 0.275, 0.7375, 1));
  p = p.concat([
    [params.hf - params.hb, bs + params.wf + bs + params.wb],
    [params.hf + bs, bs + params.wf + bs + params.wb],
    [params.hf + bs, bs],
    [0, bs]
  ]);
  p = p.concat(
    curve([0, bs - ar], ar, 0.275, 0.75, 1),
    curve([ohc[0], (-2 * chc[1]) - ohc[1]], or, 0.75, 1.25, 0.5),
    curve([-chc[0], -chc[1]], params.chr, 0.75, 0.25, 0.5));
  p = linear_extrude({height: bs}, polygon(p));
  //p = union(p, cushions);
  p = difference(p, leads);
  p = p.setColor([1, 1, 1]);
  if (show_buttons) p = union(p, buttons);
  return p;
}
