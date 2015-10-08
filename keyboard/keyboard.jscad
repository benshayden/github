'use strict';
// Everything is in mm.

var BONE_DIAMETER = 3;
var BUTTON_SIDE = 8;
var BUTTON_SEAT = [3, 1, 1.5];
var LEAD_HEIGHT = 1.5;
var COLOR = [0.4, 0.5, 0.9];
var BONE_COLOR = [1, 1, 1];
var BASE_BONES = 11;
var BASE_WIDTH = 50;

var HANDS = {left: 1, right: 1};
var BONE_RADIUS = BONE_DIAMETER / 2;
var BASE_HEIGHT = BASE_WIDTH / Math.sqrt(2);
var BASE_LENGTH = (BASE_BONES + 0.5) * 2 * BONE_DIAMETER;
var cylinder, cube, sphere, linear_extrude, union, polygon;

function getParameterDefinitions() {
  var params = [];
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
   dome = dome.center().translate([-BUTTON_SIDE / 4, BUTTON_SIDE / 2, BUTTON_SIDE / 2]);
   dome = dome.rotateZ(-90);
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

function makeBone() {
  var bone = cylinder({r: BONE_RADIUS, h: 1});
  bone = bone.rotateX(-90);
  bone = bone.translate([(BUTTON_SIDE / 2), 0, (BUTTON_SIDE / 2)]);
  return bone.setColor(COLOR);
}

function makeThumb(height, frontWidth, backWidth, wire, corner, bone) {
  bone = bone.setColor(COLOR);
  var width = frontWidth + backWidth;
  var thumb = [];
  
  var piece = cube({size: [BUTTON_SIDE, BUTTON_SIDE + height, BUTTON_SIDE]});
  piece = piece.subtract(wire.rotateZ(-90).translate([0, (BUTTON_SIDE + height) / 2, 0]));
  piece = piece.translate([0, BUTTON_SIDE, 0]);
  thumb.push(piece.setColor(COLOR));
  
  piece = piece.translate([BUTTON_SIDE + width + BUTTON_SIDE, 0]);
  piece = piece.subtract(piece.translate([2, 0]));
  thumb.push(piece.setColor(COLOR));

  piece = cube({size: [BUTTON_SIDE + width, BUTTON_SIDE, BUTTON_SIDE]});
  piece = piece.subtract(wire.translate([(BUTTON_SIDE / 2) + frontWidth, 0, 0]));
  piece = piece.translate([BUTTON_SIDE, 0, 0]);
  thumb.push(piece.setColor(COLOR));
  
  piece = piece.translate([0, BUTTON_SIDE + height + BUTTON_SIDE]);
  thumb.push(piece.setColor(COLOR));
  
  piece = corner.rotateZ(180);
  piece = piece.translate([BUTTON_SIDE, BUTTON_SIDE, 0]);
  thumb.push(piece.setColor(COLOR));
  
  piece = corner.rotateZ(90);
  piece = piece.translate([BUTTON_SIDE, BUTTON_SIDE + height + BUTTON_SIDE]);
  thumb.push(piece.setColor(COLOR));
  
  piece = corner.scale([1 / 4, 1, 1]);
  piece = piece.translate([BUTTON_SIDE + width + BUTTON_SIDE, BUTTON_SIDE + height + BUTTON_SIDE]);
  thumb.push(piece.setColor(COLOR));
  
  piece = corner.rotateZ(-90);
  piece = piece.scale([1 / 4, 1, 1]);
  piece = piece.translate([BUTTON_SIDE + width + BUTTON_SIDE, BUTTON_SIDE]);
  thumb.push(piece.setColor(COLOR));

  thumb = union(thumb);
  
  piece = bone.scale([1, BUTTON_SIDE + height + BUTTON_SIDE + (BUTTON_SIDE / 2), 1]);
  piece = piece.translate([-BUTTON_SIDE / 2, 0]);
  thumb = thumb.subtract(piece);

  piece = bone.rotateZ(-90);
  piece = piece.scale([BUTTON_SIDE + width, 1, 1]);
  piece = piece.translate([0, height + (3 * BUTTON_SIDE)]);
  thumb = thumb.subtract(piece);

  return thumb;
}

function makeBase(bone) {
  var base = cube({size: [BASE_LENGTH, BASE_HEIGHT, BASE_HEIGHT]});
  base = base.rotateX(-45);
  base = base.intersect(cube({size: [BASE_LENGTH, BASE_WIDTH, BASE_HEIGHT]}));
  var piece = bone.rotateZ(90);
  piece = piece.translate([1, (BASE_WIDTH / 2) - (BUTTON_SIDE / 2), (BUTTON_SIDE / 2) - 0.5]);
  piece = piece.scale([BASE_LENGTH, 1, 1]);
  base = base.subtract(piece);
  
  piece = cube({size: [BONE_DIAMETER, (BASE_WIDTH / 2) + (1.5 * BONE_DIAMETER), BASE_HEIGHT]});
  piece = piece.translate([BONE_DIAMETER, 0, BONE_DIAMETER]);
  piece = piece.union(piece.translate([BONE_DIAMETER * 2, (BASE_WIDTH / 2) - (1.5 * BONE_DIAMETER), 0]));
  for (var i = 0; i < Math.ceil(BASE_BONES / 2); ++i) {
    base = base.subtract(piece);
    piece = piece.translate([BONE_DIAMETER * 4, 0, 0]);
  }
  
  piece = cube({size: [BONE_DIAMETER, BASE_WIDTH / 2, BONE_DIAMETER]});
  piece = piece.translate([0, BASE_WIDTH / 2, 2 * BONE_DIAMETER]);
  base = base.subtract(piece);
  
  piece = piece.translate([BASE_LENGTH - BONE_DIAMETER, 0, 0]);
  if ((BASE_BONES % 2) === 0) {
      piece = piece.translate([0, -BASE_WIDTH / 2, 0]);
  }
  base = base.subtract(piece);

  return base.setColor(COLOR);
}

function makeBaseBones(base, bone) {
  bone = bone.setColor(BONE_COLOR);
  var piece = bone.rotateZ(90);
  piece = piece.translate([1, (BASE_WIDTH / 2) - (BUTTON_SIDE / 2), (BUTTON_SIDE / 2) - 0.5]);
  piece = piece.scale([BASE_LENGTH, 1, 1]);
  base = base.union(piece);
  piece = piece.scale([(BASE_WIDTH / 3) / BASE_LENGTH, 1, 1]);
  piece = piece.rotateZ(90);
  piece = piece.translate([(BASE_WIDTH / 2) + BONE_RADIUS, BASE_WIDTH / 2, 0]);
  base = base.union(piece);
  piece = piece.translate([BASE_LENGTH - BONE_DIAMETER, 0, 0]);
  if ((BASE_BONES % 2) == 0) {
    piece = piece.translate([0, -BASE_WIDTH / 3, 0]);
  }
  base = base.union(piece);
  piece = bone.scale([1, (BASE_WIDTH / 2) + (1.5 * BONE_DIAMETER), 1]);
  piece = piece.translate([(1.5 * BONE_DIAMETER) - (BUTTON_SIDE / 2), 0, 0]);
  piece = piece.union(piece.rotateX(90).translate([0, (2 * BONE_DIAMETER) + 1 + (BASE_WIDTH / 2), BONE_DIAMETER]));
  piece = piece.union(piece.rotateX(-90).translate([2 * BONE_DIAMETER, (BASE_WIDTH / 2) - 1 - (2 * BONE_DIAMETER), (2.5 * BONE_DIAMETER) + (BASE_WIDTH / 2)]));
  piece = piece.intersect(cube({size: [BONE_DIAMETER * 6, BASE_WIDTH, BASE_WIDTH / 2]}).setColor(BONE_COLOR));
  for (var i = 0; i < Math.ceil(BASE_BONES / 2); ++i) {
    base = base.union(piece);
    piece = piece.translate([BONE_DIAMETER * 4, 0, 0]);
  }
  base = base.intersect(cube({size: [BASE_LENGTH, BASE_WIDTH, BASE_WIDTH / 2]}).setColor(COLOR));
  return base;
}

function makeButton() {
  var button = [];
  var piece = cube({size: [BUTTON_SIDE, BUTTON_SIDE, 2]});
  button.push(piece.setColor(0, 0, 0));

  piece = cylinder({r: 2, h: 2});
  piece = piece.translate([BUTTON_SIDE / 2, BUTTON_SIDE / 2, 2]);
  button.push(piece.setColor(0.5, 0.5, 0.5));

  piece = cube({size: [1, 0.5, BUTTON_SIDE]});
  piece = piece.translate([0, BUTTON_SIDE / 2, -BUTTON_SIDE]);
  piece = piece.setColor(1, 0.7, 0.1);
  button.push(piece);

  piece = piece.translate([BUTTON_SIDE - 1, 0, 0]);
  button.push(piece);

  button = union(button);
  return button;
}

function makeHeatShrink() {
  var heatShrink = cylinder({h: 1, r: Math.sqrt(2) * BUTTON_SIDE});
  return heatShrink.setColor(0, 0, 0, 0.4);
}

function makeFinger(frontHeight, backHeight, frontWidth, backWidth, wire, dome, corner, bone) {
  bone = bone.setColor(COLOR);
  var finger = [];
  
  var piece = cube({size: [BUTTON_SIDE, BUTTON_SIDE + frontHeight, BUTTON_SIDE]});
  piece = piece.translate([0, BUTTON_SIDE, 0]);
  piece = piece.subtract(wire.rotateZ(-90).translate([0, (1.5 * BUTTON_SIDE) + frontHeight]));
  finger.push(piece.setColor(COLOR));
  
  piece = corner.rotateZ(180);
  piece = piece.translate([BUTTON_SIDE, BUTTON_SIDE]);
  finger.push(piece);
  
  piece = dome.translate([0, 2 * BUTTON_SIDE + frontHeight]);
  finger.push(piece);
  
  piece = cube({size: [frontWidth + BUTTON_SIDE + backWidth, BUTTON_SIDE, BUTTON_SIDE]}).translate([0, 0, 0]);
  piece = piece.translate([BUTTON_SIDE, 0]);
  piece = piece.subtract(wire.translate([frontWidth + (1.5 * BUTTON_SIDE), 0]));
  finger.push(piece.setColor(COLOR));
  
  piece = corner.rotateZ(-90);
  piece = piece.translate([BUTTON_SIDE + frontWidth + BUTTON_SIDE + backWidth, BUTTON_SIDE]);
  finger.push(piece);
  
  var backX = BUTTON_SIDE + frontWidth + BUTTON_SIDE + backWidth;
  piece = cube({size: [BUTTON_SIDE, BUTTON_SIDE + backHeight, BUTTON_SIDE]});
  piece = piece.translate([backX, BUTTON_SIDE]);
  piece = piece.subtract(wire.rotateZ(-90).translate([backX, (1.5 * BUTTON_SIDE) + backHeight]));
  finger.push(piece.setColor(COLOR));
  
  piece = dome.translate([backX, 2 * BUTTON_SIDE + backHeight]);
  finger.push(piece);

  finger = union(finger);
  
  piece = bone.scale([1, 2 * BUTTON_SIDE + backHeight, 1]);
  piece = piece.translate([backX, 0]);
  finger = finger.subtract(piece);
  
  piece = bone.scale([1, backX + BUTTON_SIDE / 2, 1]);
  piece = piece.rotateZ(-90);
  piece = piece.translate([0, BUTTON_SIDE / 2]);
  finger = finger.subtract(piece);
  
  return finger;
}

function makeThumbButtons(height, frontWidth, backWidth, thumb, button) {
  var piece = button.rotateX(-90).rotateY(90);
  piece = piece.translate([2 * BUTTON_SIDE + frontWidth + 0.7, BUTTON_SIDE, BUTTON_SIDE]);
  thumb = thumb.union(piece);
  
  piece = button.rotateY(90);
  piece = piece.translate([BUTTON_SIDE, BUTTON_SIDE + height / 2 - 0.7, BUTTON_SIDE]);
  thumb = thumb.union(piece);
  
  piece = button.rotateX(90).rotateY(-90);
  piece = piece.translate([2 * BUTTON_SIDE + frontWidth + 0.7, 2 * BUTTON_SIDE + height]);
  thumb = thumb.union(piece);
  
  piece = cube({size: [BUTTON_SIDE, BUTTON_SIDE, BUTTON_SIDE]});
  piece = piece.translate([0, 0, -2]);
  piece = button.intersect(piece);
  piece = piece.rotateY(-90);
  piece = piece.translate([2 * BUTTON_SIDE + frontWidth + backWidth, BUTTON_SIDE + height / 2 - 0.7]);
  thumb = thumb.union(piece);
  return thumb;
}

function makeFingerButtons(frontHeight, backHeight, frontWidth, backWidth, finger, button) {
  var piece = button.rotateX(-90).rotateY(-90);
  piece = piece.translate([BUTTON_SIDE + frontWidth, BUTTON_SIDE]);
  finger = finger.union(piece);
  
  piece = button.rotateY(90).rotateX(180);
  piece = piece.translate([BUTTON_SIDE, 2 * BUTTON_SIDE + frontHeight]);
  finger = finger.union(piece);
  
  piece = button.rotateY(-90);
  piece = piece.translate([2 * BUTTON_SIDE + frontWidth + backWidth, BUTTON_SIDE + backHeight - 0.7]);
  finger = finger.union(piece);
  return finger;
}

function makeFingerBones(fingerWidth, bone) {
  bone = bone.setColor(BONE_COLOR);
  var bend = sphere({r: BONE_RADIUS, center: true}).setColor(BONE_COLOR);
  var piece = bone.rotateZ(90);
  piece = piece.translate([1, -BUTTON_SIDE / 2, 0]);
  piece = piece.scale([(BUTTON_SIDE * 1.5) + fingerWidth, 1, 1]);
  piece = piece.translate([BUTTON_SIDE, 0, 0]);
  piece = piece.union(piece.rotateZ(90).translate([fingerWidth + (BUTTON_SIDE * 2.5), -BUTTON_SIDE, 0]));
  piece = piece.union(bend.translate([fingerWidth + (2.5 * BUTTON_SIDE), 0, BUTTON_SIDE / 2]));
  return piece;
}

function makeThumbBones(thumb, bone) {
  bone = bone.setColor(BONE_COLOR);
  var height = thumb.getBounds()[1].y;
  var piece = bone.translate([-BUTTON_SIDE / 2, 0, 0]);
  piece = piece.scale([1, height - (BUTTON_SIDE / 2), 1]);
  thumb = thumb.union(piece);
  piece = bone.rotateZ(90);
  piece = piece.translate([1, height - BUTTON_SIDE, 0]);
  piece = piece.scale([2 * BUTTON_SIDE, 1, 1]);
  thumb = thumb.union(piece);
  piece = sphere({r: BONE_RADIUS, center: true}).setColor(BONE_COLOR);
  piece = piece.translate([0, height - (BUTTON_SIDE / 2), BUTTON_SIDE / 2]);
  thumb = thumb.union(piece);
  return thumb;
}

function rad2deg(rad) {
    return rad * 180 / Math.PI;
}

function makeConnectionBone(start, end, bone) {
  bone = bone.setColor(BONE_COLOR).center();
  var boneLength = Math.sqrt(
      Math.pow(end[0] - start[0], 2) +
      Math.pow(end[1] - start[1], 2) +
      Math.pow(end[2] - start[2], 2));
  bone = bone.scale([1, boneLength, 1]);
  bone = bone.translate([0, boneLength / 2, 0]);
  bone = bone.rotateX(90);
  var yAngle = rad2deg(Math.acos((end[2] - start[2]) / boneLength));
  var zAngle = 0;
  if (start[0] === end[0]) {
    zAngle = Math.sign(end[1] - start[1]) * 90;
  } else if (end[0] > start[0]) {
    zAngle = rad2deg(Math.atan((end[1] - start[1]) / (end[0] - start[0])));
  } else {
    zAngle = rad2deg(Math.atan((end[1] - start[1]) / (end[0] - start[0]))) + 180;
  }
  bone = rotate([0, yAngle, zAngle], bone);
  bone = bone.translate(start);
  var bend = sphere({r: BONE_RADIUS, center: true}).setColor(BONE_COLOR);
  bone = bone.union(bend.translate(start));
  bone = bone.union(bend.translate(end));
  return bone;
}

function makeThumbConnectionBone(left, base, thumb, bone) {
  var thumbBounds = thumb.getBounds();
  var baseBounds = base.getBounds();
  var thumbBone = [
      thumbBounds[left ? 1 : 0].x + (BONE_RADIUS * (left ? -1 : 1)),
      thumbBounds[1].y - (BUTTON_SIDE / 2),
      thumbBounds[0].z
  ];
  var baseBone = [
      baseBounds[left ? 1 : 0].x + (1.5 * BONE_DIAMETER * (left ? -1 : 1)),
      baseBounds[0].y + (baseBounds[1].y / 2) + BONE_DIAMETER * (left ? -1 : 1),
      baseBounds[1].z
  ];
  return makeConnectionBone(baseBone, thumbBone, bone);
}

function makeFingerConnectionBone(left, digit, base, finger, bone) {
  var fingerBounds = finger.getBounds();
  var baseBounds = base.getBounds();
  var fingerBone = [
      fingerBounds[left ? 1 : 0].x + (BONE_DIAMETER * (left ? -1 : 1)),
      fingerBounds[1].y - BUTTON_SIDE,
      fingerBounds[0].z + BONE_RADIUS
  ];
  var baseBone = [
      baseBounds[left ? 1 : 0].x + (1.5 * BONE_DIAMETER * (left ? -1 : 1)),
      baseBounds[0].y + (baseBounds[1].y / 2) + BONE_DIAMETER * (left ? -1 : 1),
      baseBounds[1].z
  ];
  return makeConnectionBone(baseBone, fingerBone, bone);
  return bone;
}

function main(params) {
  var wire = makeWire();
  var dome = makeDome();
  var corner = makeCorner();
  var bone = makeBone();
  var button = makeButton();

  var world = [];

  for (var hand in HANDS) {
    var left = hand === 'left';
    var thumbHeight = params[hand + 'ThumbHeight'];
    var thumbFrontWidth = params[hand + 'ThumbFrontWidth'];
    var thumbBackWidth = params[hand + 'ThumbBackWidth'];
    var thumb = makeThumb(thumbHeight, thumbFrontWidth, thumbBackWidth, wire, corner, bone);
    var base = makeBase(bone);
    var thumbBounds = thumb.getBounds();
    var baseBounds = base.getBounds();

    if (params.displayMode === 'visual') {
      thumb = makeThumbButtons(thumbHeight, thumbFrontWidth, thumbBackWidth, thumb, button);
      base = makeBaseBones(base, bone);
      thumb = makeThumbBones(thumb, bone);
      thumb = thumb.rotateX(90);
      
      if (left) {
        thumb = thumb.rotateZ(180);
        base = base.rotateZ(180);
        thumb = thumb.translate([-BUTTON_SIDE, BUTTON_SIDE, BASE_HEIGHT + BUTTON_SIDE]);
        base = base.translate([-2 * BUTTON_SIDE, BASE_WIDTH]);
      } else {
        thumb = thumb.translate([BUTTON_SIDE, 2 * BUTTON_SIDE, BASE_HEIGHT + BUTTON_SIDE]);
        base = base.translate([2 * BUTTON_SIDE, 0]);
      }
      world.push(makeThumbConnectionBone(left, base, thumb, bone));
    } else {
      thumb = thumb.rotateZ(90);
      thumb = thumb.translate([thumbBounds[1].y, BASE_WIDTH + 1, 0]);
      if (!left) {
        thumb = thumb.rotateZ(180);
        thumb = thumb.translate([-2, 0]);
        base = base.rotateZ(180);
        base = base.translate([-2, 0]);
      }
    }
    world.push(thumb);
    world.push(base);

    for (var digit = 1; digit < 5; ++digit) {
      var frontHeight = params[hand + digit + 'FrontHeight'];
      var backHeight = params[hand + digit + 'BackHeight'];
      var frontWidth = params[hand + digit + 'FrontWidth'];
      var backWidth = params[hand + digit + 'BackWidth'];
      var finger = makeFinger(frontHeight, backHeight, frontWidth, backWidth, wire, dome, corner, bone);
      var fingerBounds = finger.getBounds();

      if (params.displayMode === 'visual') {
        finger = makeFingerButtons(frontHeight, backHeight, frontWidth, backWidth, finger, button);
        finger = finger.union(makeFingerBones(frontWidth + backWidth, bone));
        finger = finger.rotateX(90).rotateZ(-90);
        finger = finger.translate([
              (left ? -1 : 1) * (thumbBounds[1].x + BUTTON_SIDE * (2 * digit + !left)),
              fingerBounds[1].x - fingerBounds[0].x,
              BASE_HEIGHT + BUTTON_SIDE]);
        world.push(makeFingerConnectionBone(left, digit, base, finger, bone));
      } else {
        finger = finger.rotateZ(((digit % 2) ? -1 : 1) * 90);
        if (digit === 1) {
          finger = finger.translate([0, -2]);
        } else if (digit === 2) {
          finger = finger.translate([37, -47]);
        } else if (digit === 3) {
          finger = finger.translate([39, -2]);
        } else if (digit === 4) {
          finger = finger.translate([76, -47]);
        }
        if (!left) {
          finger = finger.rotateZ(180);
          finger = finger.translate([-2, 0]);
        }
      }
      world.push(finger);
    }
  }
  return world;
}
