var show_buttons = false;

function getParameterDefinitions() {
  return [
    {name: 'h', caption: 'height mm:', type: 'float', initial: 30},
    {name: 'wf', caption: 'front width mm:', type: 'float', initial: 4},
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

var chr = 1.4;  // coat hanger radius mm
var cha = 4;  // coat hanger angle rad
var bs = 8;  // button length and width mm
var cush = [3, 1, 1.5];  // cushion dimensions mm
var or = 1;  // overhang radius mm
var bbh = 3;  // button base height mm
var bh = 2;  // button height mm

function button() {
  var b = cube({size: [bs, bbh, bs]}).setColor([0, 0, 0]);
  b = union(b, cylinder({r: 2, h: bh, center: true}).setColor([0.5, 0.5, 0.5]).rotateX(90).translate([bs / 2, 4, (bs / 2)]));
  return b;
}
function lead() {
  var l = linear_extrude({height: bs}, polygon([[0.5, 0], [0, 0.5], [0, 2.5], [0.5, 3], [1, 2.5], [1, 0.5]])).rotateX(90).translate([0, bs, -0.5]);
  l = union(l, l.translate([0, 0, bs - 2]));
  return l;
}

var thin = 2;

function main(params) {
  params.h += (bbh + bh) * 2;
  params.wf += bbh + bh;
  params.wb += bbh + bh;
  var icha = (TAU - cha) / 2;
  var chc = [chr * Math.cos(icha), chr * Math.sin(icha)];
  var ohc = [or * Math.cos(icha), or * Math.sin(icha)];
  var ar = (bs / 2) + chc[1] + ohc[1]; // arch radius
  var hollow = [[0, 0], [params.wf + bs + params.wb, 0], [params.wf + bs + params.wb, params.h], [0, params.h]];
  hollow = linear_extrude({height: bs}, polygon(hollow));
  var l = lead();
  var leads = union(
    l.translate([params.wf + (bs / 2), -bs]),
    l.translate([params.wf + (bs / 2), params.h]),
    l.rotateZ(90).translate([0, params.h / 2]),
    l.rotateZ(90).translate([params.wf + bs + params.wb + bs, params.h / 2]));
  var b = button();
  var buttons = union(
    b.translate([params.wf, 0]),
    b.rotateZ(180).translate([params.wf + bs, params.h]),
    b.rotateZ(-90).translate([0, (params.h / 2) + (bs / 2)]),
    b.rotateZ(90).translate([params.wf + bs + params.wb, (params.h / 2) - (bs / 2)]));
  var p = [];
  p = p.concat(curve([params.wf + bs + params.wb + thin - ar, params.h + bs + chr], chr, 0.75, 1.25, 0.5));
  p = p.concat(curve([params.wf + bs + params.wb + thin - ar - 1, params.h + bs + (2 * chr) + 0.7], or, 0.75, 0.25, 0.5));
  p = p.concat(curve([params.wf + bs + params.wb + thin - ar, params.h + bs - chr], ar, 0.25, 0, 1));
  
  p = p.concat(curve([params.wf + bs + params.wb + thin - ar, chr - bs], ar, 0, -0.25, 1));
  p = p.concat(curve([params.wf + bs + params.wb + thin - ar - 1, -bs - (2 * chr) - 0.7], or, 0.75, 0.25, 0.5));
  p = p.concat(curve([params.wf + bs + params.wb + thin - ar, -chr - bs], chr, 0.75, 1.25, 0.5));

  p = p.concat(curve([0, -bs - chr], chr, 0.25, 0.75, 0.5));
  p = p.concat(curve([1, -bs - (2 * chr) - 0.7], or, 0.25, -0.25, 0.5));
  p = p.concat(curve([ar - bs, -bs + chr], ar, 0.75, 0.5, 1));

  p = p.concat(curve([ar - bs, params.h + bs - chr], ar, 0.5, 0.25, 1));
  p = p.concat(curve([1, params.h + bs + (2 * chr) + 0.7], or, 0.25, -0.25, 0.5));
  p = p.concat(curve([0, params.h + bs + chr], chr, 0.25, 0.75, 0.5));
  p = linear_extrude({height: bs}, polygon(p));
  p = difference(p, hollow);
  p = difference(p, leads);
  p = p.setColor([1, 1, 1]);
  if (show_buttons) p = union(p, buttons);
  return p;
}
