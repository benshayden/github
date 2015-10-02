// http://openjscad.org/

// Everything is in mm.

var HANDS = {left: 1, right: 1};
var BONE_RADIUS = 1.4;
var BUTTON_SIDE = 8;
var BUTTON_SEAT = [3, 1, 1.5];
var LEAD_HEIGHT = 1.5;
var COLOR = [.7, .8, 1];

function getParameterDefinitions() {
  var params = [];
  echo(HANDS);
  params.push({name: 'displayMode', type: 'choice', values: ['visual', 'printable']});
  for (var hand in {left: 1, right: 1}) {
    params.push({name: hand + 'ThumbHeight', type: 'float', initial: 20});
    params.push({name: hand + 'ThumbFrontWidth', type: 'float', initial: 5});
    params.push({name: hand + 'ThumbBackWidth', type: 'float', initial: 7});
    for (var digit = 1; digit < 5; ++digit) {
      params.push({name: hand + digit + 'FrontHeight', type: 'float', initial: 4});
      params.push({name: hand + digit + 'BackHeight', type: 'float', initial: 7});
      params.push({name: hand + digit + 'FrontWidth', type: 'float', initial: 4});
      params.push({name: hand + digit + 'BackWidth', type: 'float', initial: 6});
    }
  }
  return params;
}

function makeDome() {
   var c0 = cylinder({r: BUTTON_SIDE / 2, h: BUTTON_SIDE});
   var c1 = c0.rotateX(90).translate([0, BUTTON_SIDE / 2, BUTTON_SIDE / 2]);
   var q = cube({size:[BUTTON_SIDE, BUTTON_SIDE, BUTTON_SIDE]}).translate([0, -BUTTON_SIDE / 2, 0]);
   var dome = c0.intersect(c1).subtract(q);
   dome = dome.center().translate([-BUTTON_SIDE / 4, 0, BUTTON_SIDE / 2]);
   return dome.setColor(COLOR);
}

function makeWire() {
  var wire = [[0.5, 0], [0, 0.5], [0, LEAD_HEIGHT + 0.5], [0.5, LEAD_HEIGHT + 1], [1, LEAD_HEIGHT + 0.5], [1, 0.5]];
  wire = linear_extrude({height: BUTTON_SIDE}, polygon(wire)).rotateX(90).translate([0,BUTTON_SIDE, -0.5]);
  wire = union(wire, wire.translate([0, 0, BUTTON_SIDE - LEAD_HEIGHT]));
  return wire.setColor(COLOR);
}

function makeCorner() {
  var corner = cylinder({r: BUTTON_SIDE, h: BUTTON_SIDE});
  corner = corner.intersect(cube({size:[BUTTON_SIDE, BUTTON_SIDE, BUTTON_SIDE]}));
  return corner.setColor(COLOR);
}

function makeThumb(height, frontWidth, backWidth, wire, corner) {
  var width = frontWidth + backWidth;
  var thumb = [];
  
  var piece = cube({size: [BUTTON_SIDE, BUTTON_SIDE + height, BUTTON_SIDE]});
  piece = piece.subtract(wire.rotateZ(-90).translate([0, (BUTTON_SIDE + height) / 2, 0]));
  piece = piece.translate([0, BUTTON_SIDE, 0])
  thumb.push(piece);
  
  piece = cube({size: [BUTTON_SIDE + width, BUTTON_SIDE, BUTTON_SIDE]});
  piece = piece.subtract(wire.translate([(BUTTON_SIDE / 2) + frontWidth, 0, 0]));
  piece = piece.translate([BUTTON_SIDE, 0, 0]);
  thumb.push(piece);
  
  piece = corner.rotateZ(180);
  piece = piece.translate([BUTTON_SIDE, BUTTON_SIDE, 0]);
  thumb.push(piece);
  //thumb.push(thumbSide.translate([0, 0, 0]));
  //thumb.push(thumbSide.translate([0, 0, 0]));
  //thumb.push(cube({size: [2, BUTTON_SIDE + height, BUTTON_SIDE]}).translate([0, 0, 0]));
  thumb = union(thumb);
  //thumb = thumb.subtract(wire.translate([0, 0, 0]));
  //thumb = thumb.subtract(wire.translate([0, 0, 0]));
  //thumb = thumb.subtract(wire.translate([0, 0, 0]));
  return thumb.setColor(COLOR);
}

function makeBase() {
  var base = cube({size: [1, 1, 1]});
  base = base.subtract(cylinder({r: 1, h: 1}));
  return base;
}

function makeFinger(frontHeight, backHeight, frontWidth, backWidth, wire, dome) {
  var finger = [];
  finger.push(cube({size: [BUTTON_SIDE, BUTTON_SIDE + frontHeight, BUTTON_SIDE]}).translate([0, 0, 0]));
  finger.push(cube({size: [frontWidth + BUTTON_SIDE + backWidth, BUTTON_SIDE, BUTTON_SIDE]}).translate([0, 0, 0]));
  finger.push(cube({size: [BUTTON_SIDE, BUTTON_SIDE + backHeight, BUTTON_SIDE]}).translate([0, 0, 0]));
  finger.push(dome.translate([0, 0, 0]));
  finger.push(dome.translate([0, 0, 0]));
  finger = union(finger);
  finger = finger.subtract(wire.translate([0, 0, 0]));
  finger = finger.subtract(wire.translate([0, 0, 0]));
  finger = finger.subtract(wire.translate([0, 0, 0]));
  finger = finger.subtract(bone.translate([0, 0, 0]));
  finger = finger.subtract(bone.translate([0, 0, 0]));
  return finger;
}

function main(params) {
  var wire = makeWire();
  var dome = makeDome();
  var corner = makeCorner();
  var world = [];
  return makeThumb(20, 3, 6, wire, corner);
  for (var hand in HANDS) {
    var thumb = makeThumb(
      params[hand + 'ThumbHeight'],
      params[hand + 'ThumbFrontWidth'],
      params[hand + 'ThumbBackWidth'],
      wire);
    var base = makeBase();
    if (params.displayMode === 'visual') {
      // todo add buttons to thumb
      // todo position thumb, base
    } else {
      // todo position thumb, base
    }
    world.push(thumb);
    world.push(base);

    for (var digit = 1; digit < 5; ++digit) {
      var finger = makeFinger(
        params[hand + digit + 'FrontHeight'],
        params[hand + digit + 'BackHeight'],
        params[hand + digit + 'FrontWidth'],
        params[hand + digit + 'BackWidth'],
        wire,
        dome);
      if (params.displayMode === 'visual') {
        // todo add buttons to finger
        // todo position finger
      } else {
        // todo position finger
      }
      world.push(finger);
    }
  }
  return world;
}
