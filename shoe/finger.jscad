// You shouldn't need to change these, but you can if you want.
var show_buttons = false;
var chr = 1.4;  // coat hanger radius mm
var cha = 4;  // coat hanger angle rad
var bs = 8;  // button length and width mm
var cush = [3, 1, 1.5];  // cushion dimensions mm
var or = 1;  // overhang radius mm
var bbh = 3;  // button base height mm
var bh = 2;  // button height mm
var lh = 1.5;  // lead height mm
var the_color = [1, 1, 1];

function getParameterDefinitions() {
  return [
    {name: 'lv', caption: 'number of buttons:', type: 'choice', values: [0, 1], captions: ['3 buttons', '5 buttons']},
    {name: 'hf', caption: 'front height mm:', type: 'float', initial: 4},
    {name: 'hb', caption: 'back height mm:', type: 'float', initial: 7},
    {name: 'wf', caption: 'front width mm:', type: 'float', initial: 4},
    {name: 'wb', caption: 'back width mm:', type: 'float', initial: 6}];
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

function button() {
  var b = cube({size: [bs, bbh, bs]}).setColor([0, 0, 0, 0.7]);
  b = union(b, cylinder({r: 2, h: bh, center: true}).setColor([0.5, 0.5, 0.5, 0.7]).rotateX(90).translate([bs / 2, 4, (bs / 2)]));
  return b;
}

function lead() {
  var l = linear_extrude({height: bs}, polygon([[0.5, 0], [0, 0.5], [0, lh + 0.5], [0.5, lh + 1], [1, lh + 0.5], [1, 0.5]])).rotateX(90).translate([0, bs, -0.5]);
  l = union(l, l.translate([0, 0, bs - lh]));
  return l;
}

function main(params) {
  params.lv = parseInt(params.lv, 10);
  params.hf += bbh + bh - (bs / 2);
  params.hb += bbh + bh - (bs / 2);
  params.wf += bbh + bh - (bs / 2);
  params.wb += bbh + bh - (bs / 2);
  var icha = (TAU - cha) / 2;
  var chc = [chr * Math.cos(icha), chr * Math.sin(icha)];
  var ohc = [or * Math.cos(icha), or * Math.sin(icha)];
  var ar = (bs / 2) + chc[1] + ohc[1]; // arch radius
  var l = lead();
  var leads = union(
    l.translate([(bs / 2), 0, 0]),
    l.translate([bs / 2 + params.hf - params.hb, bs + params.wf + bs + params.wb, 0]),
    l.rotateZ(90).translate([bs + params.hf + bs, bs + params.wf + (bs / 2), 0]));
  if (params.lv) {
    leads = union(leads, 
      l.translate([-0.5 - (bs / 2), -3, 0]),
      l.translate([bs / 2 + params.hf - params.hb - bs, bs + 3 + params.wf + bs + params.wb, 0]));
  }
  var cushion = cube({size: cush});
  cushion = union(cushion, cushion.translate([0, 0, bs - cush[2]]));
  var cushions = union(
    cushion.translate([0, bs, 0]),
    cushion.translate([params.hf - params.hb, bs - cush[1] + params.wf + bs + params.wb, 0]),
    cushion.rotateZ(90).translate([bs + params.hf, bs + params.wf, 0]));
  if (params.lv) {
    // TODO
    cushions = union(cushions,
      cushion.translate([0, 0, 0]),
      cushion.translate([0, 0, 0]));
  }
  var b = button();
  var buttons = union(
    b.translate([0, bs, 0]),
    b.rotateX(180).translate([params.hf - params.hb, bs + params.wf + bs + params.wb, bs]),
    b.rotateZ(90).translate([bs + params.hf, bs + params.wf, 0]));
  if (params.lv) {
    buttons = union(buttons,
      b.translate([-bs, bs - 3, 0]),
      b.rotateX(180).translate([params.hf - params.hb - bs, bs + 3 + params.wf + bs + params.wb, bs]));
  }
  var pl = [params.hf - params.hb, bs + params.wf + bs + params.wb + bs];
  if (params.lv) {
    pl[0] -= bs;
    pl[1] += 3;
  }
  var pf0 = params.lv ? -bs : 0;
  var pf1 = params.lv ? -3 : 0;
  var p = (params.lv ? [[-bs, -3], [4, -3], [4, 0]] : [[0, 0]]).concat(
    curve([bs + params.hf, bs], bs, 0.75, 1, 1),
    curve([bs + params.hf, bs + params.wf + bs + params.wb], bs, 0, 0.25, 1),
    params.lv ? [[pl[0] + bs + 4, pl[1] - 3], [pl[0] + bs + 4, pl[1]]] : [],
    [pl],
    curve([pl[0] - chc[0], pl[1] + chc[1]], chr, 0.75, 0.25, 0.5),
    curve([pl[0] + ohc[0], pl[1] + (2 * chc[1]) + ohc[1]], or, 0.75, 1.25, 0.5),
    curve([pl[0], pl[1] - bs + ar], ar, 0.275, 0.7375, 1), 
    params.lv ? [
      [params.hf - params.hb - bs, bs + params.wf + bs + params.wb + 3],
      [params.hf - params.hb, bs + params.wf + bs + params.wb + 3]] : [], [
    [params.hf - params.hb, bs + params.wf + bs + params.wb],
    [params.hf + bs, bs + params.wf + bs + params.wb],
    [params.hf + bs, bs],
    [0, bs]],
    params.lv ? [[0, 5], [-bs, 5]] : [],
    curve([pf0, bs - ar + pf1], ar, 0.275, 0.75, 1),
    curve([ohc[0] + pf0, (-2 * chc[1]) - ohc[1] + pf1], or, 0.75, 1.25, 0.5),
    curve([-chc[0] + pf0, -chc[1] + pf1], chr, 0.75, 0.25, 0.5));
  p = linear_extrude({height: bs}, polygon(p));
  //p = union(p, cushions);
  p = difference(p, leads);
  p = p.setColor(the_color);
  if (show_buttons) p = union(p, buttons);
  return p;
}
