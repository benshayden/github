// http://openjscad.org/

// Everything is in mm.

var HANDS = ['left', 'right'];
var BONE_RADIUS = 1.4;
var BUTTON_SIDE = 8;
var BUTTON_SEAT = [3, 1, 1.5];
var LEAD_HEIGHT = 1.5;

function getParameterDefinitions() {
  var params = [];
  params.push({name: 'displayMode', type: 'choice', values: ['visual', 'printable']});
  HANDS.foreach(function(hand) {
    params.push({name: hand + 'ThumbHeight', type: 'float', initial: 20});
    params.push({name: hand + 'ThumbFrontWidth', type: 'float', initial: 5});
    params.push({name: hand + 'ThumbBackWidth', type: 'float', initial: 7});
    for (var digit = 1; digit < 5; ++digit) {
      params.push({name: hand + digit + 'FrontHeight', type: 'float', initial: 4});
      params.push({name: hand + digit + 'BackHeight', type: 'float', initial: 7});
      params.push({name: hand + digit + 'FrontWidth', type: 'float', initial: 4});
      params.push({name: hand + digit + 'BackWidth', type: 'float', initial: 6});
    }
  });
  return params;
}

function debug(o) {
  return o.setColor(1, 0.2, 0.2);
}

function makeDome() {
   var c0 = cylinder({r: BUTTON_SIDE / 2, h: BUTTON_SIDE});
   var c1 = c0.rotateX(90).translate([0, BUTTON_SIDE / 2, BUTTON_SIDE / 2]);
   var q = cube({size:[BUTTON_SIDE, BUTTON_SIDE, BUTTON_SIDE]}).translate([0, -BUTTON_SIDE / 2, 0]);
   var dome = c0.intersect(c1).subtract(q);
   dome = dome.center().translate([-BUTTON_SIDE / 4, 0, BUTTON_SIDE / 2]);
   return dome.setColor([.9, .9, .9]);
}

function makeWireLead() {
  var wireLead = [[0.5, 0], [0, 0.5], [0, LEAD_HEIGHT + 0.5], [0.5, LEAD_HEIGHT + 1], [1, LEAD_HEIGHT + 0.5], [1, 0.5]];
  wireLead = linear_extrude({height: BUTTON_SIDE}, polygon(wireLead)).rotateX(90).translate([0,BUTTON_SIDE, -0.5]);
  wireLead = union(wireLead, wireLead.translate([0, 0, BUTTON_SIDE - LEAD_HEIGHT]));
  return wireLead.setColor([.9, .9, .9]);
}

function makeThumb(height, frontWidth, backWidth, wireLead) {
  var width = frontWidth + backWidth;
  var thumb = [];
  thumb.push(cube({size: [BUTTON_SIDE, BUTTON_SIDE + height, BUTTON_SIDE]}).translate([0, 0, 0]));
  var thumbSide = cube({size: [BUTTON_SIDE + width, BUTTON_SIDE, BUTTON_SIDE]});
  thumb.push(thumbSide.translate([0, 0, 0]));
  thumb.push(thumbSide.translate([0, 0, 0]));
  thumb.push(cube({size: [2, BUTTON_SIDE + height, BUTTON_SIDE]}).translate([0, 0, 0]));
  thumb = union(thumb);
  thumb = thumb.subtract(wireLead.translate([0, 0, 0]));
  thumb = thumb.subtract(wireLead.translate([0, 0, 0]));
  thumb = thumb.subtract(wireLead.translate([0, 0, 0]));
  thumb = thumb.subtract(wireLead.translate([0, 0, 0]));
  return thumb;
}

function makeBase() {
  var base = cube({size: [1, 1, 1]});
  base = base.subtract(cylinder({r: 1, h: 1}));
  return base;
}

function makeFinger(frontHeight, backHeight, frontWidth, backWidth, wireLead, dome) {
  var finger = [];
  finger.push(cube({size: [BUTTON_SIDE, BUTTON_SIDE + frontHeight, BUTTON_SIDE]}).translate([0, 0, 0]));
  finger.push(cube({size: [frontWidth + BUTTON_SIDE + backWidth, BUTTON_SIDE, BUTTON_SIDE]}).translate([0, 0, 0]));
  finger.push(cube({size: [BUTTON_SIDE, BUTTON_SIDE + backHeight, BUTTON_SIDE]}).translate([0, 0, 0]));
  finger.push(dome.translate([0, 0, 0]));
  finger.push(dome.translate([0, 0, 0]));
  finger = union(finger);
  finger = finger.subtract(wireLead.translate([0, 0, 0]));
  finger = finger.subtract(wireLead.translate([0, 0, 0]));
  finger = finger.subtract(wireLead.translate([0, 0, 0]));
  finger = finger.subtract(bone.translate([0, 0, 0]));
  finger = finger.subtract(bone.translate([0, 0, 0]));
  return finger;
}

function main(params) {
  var wireLead = makeWireLead();
  var dome = makeDome();
  var world = [];
  HANDS.forEach(function(hand) {
    var thumb = makeThumb(
      params[hand + 'ThumbHeight'],
      params[hand + 'ThumbFrontWidth'],
      params[hand + 'ThumbBackWidth'],
      wireLead);
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
        wireLead,
        dome);
      if (params.displayMode === 'visual') {
        // todo add buttons to finger
        // todo position finger
      } else {
        // todo position finger
      }
      world.push(finger);
    }
  });
  return world;
}
