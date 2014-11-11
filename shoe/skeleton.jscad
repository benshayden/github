var show_buttons = false;
function getParameterDefinitions() {
  return [
    {name: 'chr', caption: 'Coat hanger radius mm:', type: 'float', initial: 1.5875},
    {name: 'hf0', caption: 'Index Front height mm:', type: 'float', initial: 7},
    {name: 'hb0', caption: 'Index Back height mm:', type: 'float', initial: 9},
    {name: 'wf0', caption: 'Index Front width mm:', type: 'float', initial: 8},
    {name: 'wb0', caption: 'Index Back width mm:', type: 'float', initial: 10},
    {name: 'h1', caption: 'Thumb height mm:', type:'float', initial: 40},
    {name: 'wf1', caption: 'Thumb front width mm:', type:'float', initial: 9},
    {name: 'wb1', caption: 'Thumb back width mm:', type:'float', initial: 15}];
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

function finger(hf, hb, wf, wb, chr, chc, ohc, ar) {
  var l = lead();
  var leads = union(
    l.translate([bs / 2, 0, 0]),
    l.translate([bs / 2 + hf - hb, bs + wf + bs + wb, 0]),
    l.rotateZ(90).translate([bs + hf + bs, bs + wf + (bs / 2), 0]));
  var cushion = cube({size: cush});
  cushion = union(cushion, cushion.translate([0, 0, bs - cush[2]]));
  var cushions = union(
    cushion.translate([0, bs, 0]),
    cushion.translate([hf - hb, bs - cush[1] + wf + bs + wb, 0]),
    cushion.rotateZ(90).translate([bs + hf, bs + wf, 0]));
  var b = button();
  var buttons = union(
    b.translate([0, bs, 0]),
    b.rotateX(180).translate([hf - hb, bs + wf + bs + wb, bs]),
    b.rotateZ(90).translate([bs + hf, bs + wf, 0]));
  var p = [
    [0, 0],
    [bs + hf + bs, 0],
    [bs + hf + bs, bs + wf + bs + wb + bs],
    [hf - hb, bs + wf + bs + wb + bs]
  ];
  var pl = p[p.length - 1];
  p = p.concat(
    curve([pl[0] - chc[0], pl[1] + chc[1]], chr, 0.75, 0.25, 0.5),
    curve([pl[0] + ohc[0], pl[1] + (2 * chc[1]) + ohc[1]], or, 0.75, 1.25, 0.5),
    curve([pl[0], pl[1] - bs + ar], ar, 0.275, 0.7375, 1));
  p = p.concat([
    [hf - hb, bs + wf + bs + wb],
    [hf + bs, bs + wf + bs + wb],
    [hf + bs, bs],
    [0, bs]
  ]);
  p = p.concat(
    curve([0, bs - ar], ar, 0.275, 0.75, 1),
    curve([ohc[0], (-2 * chc[1]) - ohc[1]], or, 0.75, 1.25, 0.5),
    curve([-chc[0], -chc[1]], chr, 0.75, 0.25, 0.5));
  p = linear_extrude({height: bs}, polygon(p));
  //p = union(p, cushions);
  p = difference(p, leads);
  p = p.setColor([1, 1, 1]);
  if (show_buttons) p = union(p, buttons);
  return p;
}

var thin = 2;

function thumb(h, wf, wb, chr, chc, ohc, ar) {
  var hollow = [[0, 0], [wf + bs + wb, 0], [wf + bs + wb, h], [0, h]];
  hollow = linear_extrude({height: bs}, polygon(hollow));
  var l = lead();
  var leads = union(
    l.translate([wf + (bs / 2), -bs]),
    l.translate([wf + (bs / 2), h]),
    l.rotateZ(90).translate([0, h / 2]),
    l.rotateZ(90).translate([wf + bs + wb + bs, h / 2]));
  var b = button();
  var buttons = union(
    b.translate([wf, 0]),
    b.rotateZ(180).translate([wf + bs, h]),
    b.rotateZ(-90).translate([0, (h / 2) + (bs / 2)]),
    b.rotateZ(90).translate([wf + bs + wb, (h / 2) - (bs / 2)]));
  var p = [];
  p = p.concat(curve([wf + bs + wb + thin - ar, h + bs + chr], chr, 0.75, 1.25, 0.5));
  p = p.concat(curve([wf + bs + wb +thin - ar - 1, h + bs + (2 * chr) + 0.7], or, 0.75, 0.25, 0.5));
  p = p.concat(curve([wf + bs + wb + thin - ar, h + bs - chr], ar, 0.25, 0, 1));
  
  p = p.concat(curve([wf + bs + wb + thin - ar, chr - bs], ar, 0, -0.25, 1));
  p = p.concat(curve([wf + bs + wb +thin - ar - 1, -bs - (2 * chr) - 0.7], or, 0.75, 0.25, 0.5));
  p = p.concat(curve([wf + bs + wb + thin - ar, -chr - bs], chr, 0.75, 1.25, 0.5));

  p = p.concat(curve([0, -bs - chr], chr, 0.25, 0.75, 0.5));
  p = p.concat(curve([1, -bs - (2 * chr) - 0.7], or, 0.25, -0.25, 0.5));
  p = p.concat(curve([ar - bs, -bs + chr], ar, 0.75, 0.5, 1));

  p = p.concat(curve([ar - bs, h + bs - chr], ar, 0.5, 0.25, 1));
  p = p.concat(curve([1, h + bs + (2 * chr) + 0.7], or, 0.25, -0.25, 0.5));
  p = p.concat(curve([0, h + bs + chr], chr, 0.25, 0.75, 0.5));
  p = linear_extrude({height: bs}, polygon(p));
  p = difference(p, hollow);
  p = difference(p, leads);
  p = p.setColor([1, 1, 1]);
  if (show_buttons) p = union(p, buttons);
  return p;
}

function main(params) {
  var icha = (TAU - cha) / 2;
  var chc = [params.chr * Math.cos(icha), params.chr * Math.sin(icha)];
  var ohc = [or * Math.cos(icha), or * Math.sin(icha)];
  var ar = (bs / 2) + chc[1] + ohc[1]; // arch radius
  return union(
    finger(params.hf0, params.hb0, params.wf0, params.wb0, params.chr, chc, ohc, ar).translate([-35, -bs]),
    thumb(params.h1, params.wf1, params.wb1, params.chr, chc, ohc, ar));
}
