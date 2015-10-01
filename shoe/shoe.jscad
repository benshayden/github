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

function dome() {
  return intersect(cylinder({}), cylinder({}));
}

function wireLead() {
  return cube({});
}

function makeThumb(height, frontWidth, backWidth) {
  var thumb = [];
  thumb.push(cube({}));
  thumb.push(cube({}));
  thumb.push(cube({}));
  thumb.push(cube({}));
  thumb = union(thumb);
  thumb = thumb.subtract(wireLead);
  thumb = thumb.subtract(wireLead);
  thumb = thumb.subtract(wireLead);
  thumb = thumb.subtract(wireLead);
  return thumb;
}

function makeBase() {
  var base = cube({});
  base = base.subtract(cylinder({}));
  return base;
}

function makeFinger(frontHeight, backHeight, frontWidth, backWidth) {
  var finger = [];
  finger.push(cube({}));
  finger.push(cube({}));
  finger.push(cube({}));
  finger.push(dome);
  finger.push(dome);
  finger = union(finger);
  finger = finger.subtract(wireLead);
  finger = finger.subtract(wireLead);
  finger = finger.subtract(wireLead);
  finger = finger.subtract(bone);
  finger = finger.subtract(bone);
  return finger;
}

function main(params) {
  var world = [];
  HANDS.forEach(function(hand) {
    var thumb = makeThumb(
      params[hand + 'ThumbHeight'],
      params[hand + 'ThumbFrontWidth'],
      params[hand + 'ThumbBackWidth']);
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
        params[hand + digit + 'BackWidth']);
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
