'use strict';
// Everything is in mm.

var BONE_DIAMETER = 3;
var BUTTON_SIDE = 8;
var BUTTON_SEAT = [3, 1, 1.5];
var LEAD_HEIGHT = 1.5;
var COLOR = [0.4, 0.5, 0.9];
var BONE_COLOR = [1, 1, 1];

var HANDS = {left: 1, right: 1};
var BONE_RADIUS = BONE_DIAMETER / 2;
var cylinder, cube, sphere, linear_extrude, union, polygon;

function getParameterDefinitions() {
  var params = [];
  params.push({name: 'displayMode', type: 'choice', values: ['visual', 'printable']});
  params.push({name: 'part', type: 'choice', values: ['all', 'base', 'thumb', 'finger']});
  params.push({name: 'baseBones', type: 'float', initial: 11});
  params.push({name: 'baseWidth', type: 'float', initial: 50});
  params.push({name: 'boneDiameter', type: 'float', initial: 3});
  params.push({name: 'buttonSide', type: 'float', initial: 8});
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

function makeDome(buttonSide) {
   var c0 = cylinder({r: buttonSide / 2, h: buttonSide});
   var c1 = c0.rotateX(90).translate([0, buttonSide / 2, buttonSide / 2]);
   var q = cube({size:[buttonSide, buttonSide, buttonSide]}).translate([0, -buttonSide / 2, 0]);
   var dome = c0.intersect(c1).subtract(q);
   dome = dome.center().translate([-buttonSide / 4, buttonSide / 2, buttonSide / 2]);
   dome = dome.rotateZ(-90);
   return dome.setColor(COLOR);
}

function makeWire(buttonSide) {
  var wire = [[0.5, 0], [0, 0.5], [0, LEAD_HEIGHT + 0.5], [0.5, LEAD_HEIGHT + 1], [1, LEAD_HEIGHT + 0.5], [1, 0.5]];
  wire = linear_extrude({height: buttonSide}, polygon(wire)).rotateX(90).translate([0,buttonSide, -0.5]);
  wire = union(wire, wire.translate([0, 0, buttonSide - LEAD_HEIGHT]));
  return wire.setColor(COLOR);
}

function makeCorner(buttonSide) {
  var corner = cylinder({r: buttonSide, h: buttonSide});
  corner = corner.intersect(cube({size:[buttonSide, buttonSide, buttonSide]}));
  return corner.setColor(COLOR);
}

function makeBone(boneDiameter) {
  var bone = cylinder({r: boneDiameter / 2, h: 1});
  bone = bone.rotateX(-90);
  bone = bone.translate([(BUTTON_SIDE / 2), 0, (BUTTON_SIDE / 2)]);
  return bone.setColor(COLOR);
}

function makeThumb(height, frontWidth, backWidth, buttonSide, wire, corner, bone) {
  bone = bone.setColor(COLOR);
  var width = frontWidth + backWidth;
  var thumb = [];
  
  var piece = cube({size: [buttonSide, buttonSide + height, buttonSide]});
  piece = piece.subtract(wire.rotateZ(-90).translate([0, (buttonSide + height) / 2, 0]));
  piece = piece.translate([0, buttonSide, 0]);
  thumb.push(piece.setColor(COLOR));
  
  piece = piece.translate([buttonSide + width + buttonSide, 0]);
  piece = piece.subtract(piece.translate([2, 0]));
  thumb.push(piece.setColor(COLOR));

  piece = cube({size: [buttonSide + width, buttonSide, buttonSide]});
  piece = piece.subtract(wire.translate([(buttonSide / 2) + frontWidth, 0, 0]));
  piece = piece.translate([buttonSide, 0, 0]);
  thumb.push(piece.setColor(COLOR));
  
  piece = piece.translate([0, buttonSide + height + buttonSide]);
  thumb.push(piece.setColor(COLOR));
  
  piece = corner.rotateZ(180);
  piece = piece.translate([buttonSide, buttonSide, 0]);
  thumb.push(piece.setColor(COLOR));
  
  piece = corner.rotateZ(90);
  piece = piece.translate([buttonSide, buttonSide + height + buttonSide]);
  thumb.push(piece.setColor(COLOR));
  
  piece = corner.scale([1 / 4, 1, 1]);
  piece = piece.translate([buttonSide + width + buttonSide, buttonSide + height + buttonSide]);
  thumb.push(piece.setColor(COLOR));
  
  piece = corner.rotateZ(-90);
  piece = piece.scale([1 / 4, 1, 1]);
  piece = piece.translate([buttonSide + width + buttonSide, buttonSide]);
  thumb.push(piece.setColor(COLOR));

  thumb = union(thumb);
  
  piece = bone.scale([1, buttonSide + height + buttonSide + (buttonSide / 2), 1]);
  piece = piece.translate([-buttonSide / 2, 0]);
  thumb = thumb.subtract(piece);

  piece = bone.rotateZ(-90);
  piece = piece.scale([buttonSide + width, 1, 1]);
  piece = piece.translate([0, height + (3 * buttonSide)]);
  thumb = thumb.subtract(piece);

  return thumb;
}

function getBaseLength(baseBones) {
  return (baseBones + 0.5) * 2 * BONE_DIAMETER;
}

function makeBase(baseWidth, baseBones, bone) {
  var baseSide = baseWidth / Math.sqrt(2);
  var baseLength = getBaseLength(baseBones);
  var base = cube({size: [baseLength, baseSide, baseSide]});
  base = base.rotateX(-45);
  base = base.intersect(cube({size: [baseLength, baseWidth, baseSide]}));
  var piece = bone.rotateZ(90);
  piece = piece.translate([1, (baseWidth / 2) - (BUTTON_SIDE / 2), (BUTTON_SIDE / 2) - 0.5]);
  piece = piece.scale([baseLength, 1, 1]);
  base = base.subtract(piece);
  
  piece = cube({size: [BONE_DIAMETER, (baseWidth / 2) + (1.5 * BONE_DIAMETER), baseSide]});
  piece = piece.translate([BONE_DIAMETER, 0, BONE_DIAMETER]);
  piece = piece.union(piece.translate([BONE_DIAMETER * 2, (baseWidth / 2) - (1.5 * BONE_DIAMETER), 0]));
  for (var i = 0; i < Math.ceil(baseBones / 2); ++i) {
    base = base.subtract(piece);
    piece = piece.translate([BONE_DIAMETER * 4, 0, 0]);
  }
  
  piece = cube({size: [BONE_DIAMETER, baseWidth / 2, BONE_DIAMETER]});
  piece = piece.translate([0, baseWidth / 2, 2 * BONE_DIAMETER]);
  base = base.subtract(piece);
  
  piece = piece.translate([baseLength - BONE_DIAMETER, 0, 0]);
  if ((baseBones % 2) === 0) {
      piece = piece.translate([0, -baseWidth / 2, 0]);
  }
  base = base.subtract(piece);

  return base.setColor(COLOR);
}

function makeBaseBones(baseWidth, baseBones, base, bone) {
  bone = bone.setColor(BONE_COLOR);
  var piece = bone.rotateZ(90);
  var baseLength = getBaseLength(baseBones);
  piece = piece.translate([1, (baseWidth / 2) - (BUTTON_SIDE / 2), (BUTTON_SIDE / 2) - 0.5]);
  piece = piece.scale([baseLength, 1, 1]);
  base = base.union(piece);
  piece = piece.scale([(baseWidth / 3) / baseLength, 1, 1]);
  piece = piece.rotateZ(90);
  piece = piece.translate([(baseWidth / 2) + BONE_RADIUS, baseWidth / 2, 0]);
  base = base.union(piece);
  piece = piece.translate([baseLength - BONE_DIAMETER, 0, 0]);
  if ((baseBones % 2) === 0) {
    piece = piece.translate([0, -baseWidth / 3, 0]);
  }
  base = base.union(piece);
  piece = bone.scale([1, (baseWidth / 2) + (1.5 * BONE_DIAMETER), 1]);
  piece = piece.translate([(1.5 * BONE_DIAMETER) - (BUTTON_SIDE / 2), 0, 0]);
  piece = piece.union(piece.rotateX(90).translate([0, (2 * BONE_DIAMETER) + 1 + (baseWidth / 2), BONE_DIAMETER]));
  piece = piece.union(piece.rotateX(-90).translate([2 * BONE_DIAMETER, (baseWidth / 2) - 1 - (2 * BONE_DIAMETER), (2.5 * BONE_DIAMETER) + (baseWidth / 2)]));
  piece = piece.intersect(cube({size: [BONE_DIAMETER * 6, baseWidth, baseWidth / 2]}).setColor(BONE_COLOR));
  for (var i = 0; i < Math.ceil(baseBones / 2); ++i) {
    base = base.union(piece);
    piece = piece.translate([BONE_DIAMETER * 4, 0, 0]);
  }
  base = base.intersect(cube({size: [baseLength, baseWidth, baseWidth / 2]}).setColor(COLOR));
  return base;
}

function makeButton(buttonSide) {
  var button = [];
  var piece = cube({size: [buttonSide, buttonSide, 2]});
  button.push(piece.setColor(0, 0, 0));

  piece = cylinder({r: 2, h: 2});
  piece = piece.translate([buttonSide / 2, buttonSide / 2, 2]);
  button.push(piece.setColor(0.5, 0.5, 0.5));

  piece = cube({size: [1, 0.5, buttonSide]});
  piece = piece.translate([0, buttonSide / 2, -buttonSide]);
  piece = piece.setColor(1, 0.7, 0.1);
  button.push(piece);

  piece = piece.translate([buttonSide - 1, 0, 0]);
  button.push(piece);

  button = union(button);
  return button;
}

function makeHeatShrink() {
  var heatShrink = cylinder({h: 1, r: Math.sqrt(2) * BUTTON_SIDE});
  return heatShrink.setColor(0, 0, 0, 0.4);
}

function makeFinger(frontHeight, backHeight, frontWidth, backWidth, buttonSide, wire, dome, corner, bone) {
  bone = bone.setColor(COLOR);
  var finger = [];
  
  var piece = cube({size: [buttonSide, buttonSide + frontHeight, buttonSide]});
  piece = piece.translate([0, buttonSide, 0]);
  piece = piece.subtract(wire.rotateZ(-90).translate([0, (1.5 * buttonSide) + frontHeight]));
  finger.push(piece.setColor(COLOR));
  
  piece = corner.rotateZ(180);
  piece = piece.translate([buttonSide, buttonSide]);
  finger.push(piece);
  
  piece = dome.translate([0, 2 * buttonSide + frontHeight]);
  finger.push(piece);
  
  piece = cube({size: [frontWidth + buttonSide + backWidth, buttonSide, buttonSide]}).translate([0, 0, 0]);
  piece = piece.translate([buttonSide, 0]);
  piece = piece.subtract(wire.translate([frontWidth + (1.5 * buttonSide), 0]));
  finger.push(piece.setColor(COLOR));
  
  piece = corner.rotateZ(-90);
  piece = piece.translate([buttonSide + frontWidth + buttonSide + backWidth, buttonSide]);
  finger.push(piece);
  
  var backX = buttonSide + frontWidth + buttonSide + backWidth;
  piece = cube({size: [buttonSide, buttonSide + backHeight, buttonSide]});
  piece = piece.translate([backX, buttonSide]);
  piece = piece.subtract(wire.rotateZ(-90).translate([backX, (1.5 * buttonSide) + backHeight]));
  finger.push(piece.setColor(COLOR));
  
  piece = dome.translate([backX, 2 * buttonSide + backHeight]);
  finger.push(piece);

  finger = union(finger);
  
  piece = bone.scale([1, 2 * buttonSide + backHeight, 1]);
  piece = piece.translate([backX, 0]);
  finger = finger.subtract(piece);
  
  piece = bone.scale([1, backX + buttonSide / 2, 1]);
  piece = piece.rotateZ(-90);
  piece = piece.translate([0, buttonSide / 2]);
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
  bone = bone.rotateY(rad2deg(Math.acos((end[2] - start[2]) / boneLength)));
  var zAngle = 0;
  if (start[0] === end[0]) {
    bone = bone.rotateZ(Math.sign(end[1] - start[1]) * 90);
  } else {
    bone = bone.rotateZ(rad2deg(Math.atan2((end[1] - start[1]), (end[0] - start[0]))));
  }
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
      fingerBounds[left ? 1 : 0].x + (BUTTON_SIDE * (left ? -1 : 1) / 2),
      fingerBounds[1].y - BUTTON_SIDE,
      fingerBounds[0].z + BONE_RADIUS
  ];
  var baseBone = [
      baseBounds[left ? 0 : 1].x + (((4 * BONE_DIAMETER * (4 - digit)) + (1.5 * BONE_DIAMETER)) * (left ? 1 : -1)),
      baseBounds[0].y + (baseBounds[1].y / 2) + BONE_DIAMETER * (left ? -1 : 1),
      baseBounds[1].z
  ];
  return makeConnectionBone(baseBone, fingerBone, bone);
}

function main(params) {
  var wire = makeWire(params.buttonSide);
  var dome = makeDome(params.buttonSide);
  var corner = makeCorner(params.buttonSide);
  var bone = makeBone(params.boneDiameter);
  var button = makeButton(params.buttonSide);
  var base = makeBase(params.baseWidth, params.baseBones, bone);
  if (params.part === 'base') {
    return base;
  }
  var baseBounds = base.getBounds();

  var world = [];

  for (var hand in HANDS) {
    var left = hand === 'left';
    var thumbHeight = params[hand + 'ThumbHeight'];
    var thumbFrontWidth = params[hand + 'ThumbFrontWidth'];
    var thumbBackWidth = params[hand + 'ThumbBackWidth'];
    var thumb = makeThumb(thumbHeight, thumbFrontWidth, thumbBackWidth, params.buttonSide, wire, corner, bone);
    if (params.part === 'thumb') {
      return thumb;
    }
    var thumbBounds = thumb.getBounds();
    var handBase = base;

    if (params.displayMode === 'visual') {
      thumb = makeThumbButtons(thumbHeight, thumbFrontWidth, thumbBackWidth, thumb, button);
      handBase = makeBaseBones(params.baseWidth, params.baseBones, handBase, bone);
      thumb = makeThumbBones(thumb, bone);
      thumb = thumb.rotateX(90);
      
      if (left) {
        thumb = thumb.rotateZ(180);
        handBase = handBase.rotateZ(180);
        thumb = thumb.translate([-params.buttonSide, params.buttonSide, params.buttonSide + (params.baseWidth / 2)]);
        handBase = handBase.translate([-2 * params.buttonSide, params.baseWidth]);
      } else {
        thumb = thumb.translate([params.buttonSide, 2 * params.buttonSide, params.buttonSide + (params.baseWidth / 2)]);
        handBase = handBase.translate([2 * params.buttonSide, 0]);
      }
      world.push(makeThumbConnectionBone(left, handBase, thumb, bone));
    } else {
      thumb = thumb.rotateZ(90);
      thumb = thumb.translate([thumbBounds[1].y, params.baseWidth + 1, 0]);
      if (!left) {
        thumb = thumb.rotateZ(180);
        thumb = thumb.translate([-1, thumbBounds[1].y - params.buttonSide]);
        handBase = handBase.rotateZ(180);
        handBase = handBase.translate([-1, thumbBounds[1].y - params.buttonSide]);
      }
    }
    world.push(thumb);
    world.push(handBase);

    for (var digit = 1; digit < 5; ++digit) {
      var frontHeight = params[hand + digit + 'FrontHeight'];
      var backHeight = params[hand + digit + 'BackHeight'];
      var frontWidth = params[hand + digit + 'FrontWidth'];
      var backWidth = params[hand + digit + 'BackWidth'];
      var finger = makeFinger(frontHeight, backHeight, frontWidth, backWidth, params.buttonSide, wire, dome, corner, bone);
      if (params.part === 'finger') {
        return finger;
      }
      var fingerBounds = finger.getBounds();

      if (params.displayMode === 'visual') {
        finger = makeFingerButtons(frontHeight, backHeight, frontWidth, backWidth, finger, button);
        finger = finger.union(makeFingerBones(frontWidth + backWidth, bone));
        finger = finger.rotateX(90).rotateZ(-90);
        finger = finger.translate([
              (left ? -1 : 1) * (thumbBounds[1].x + params.buttonSide * (2 * digit + !left)),
              fingerBounds[1].x - fingerBounds[0].x,
              params.buttonSide + (params.baseWidth / 2)]);
        world.push(makeFingerConnectionBone(left, digit, handBase, finger, bone));
      } else {
        finger = finger.rotateZ(((digit % 2) ? -1 : 1) * 90);
        if (digit === 1) {
          finger = finger.translate([0, -1]);
        } else if (digit === 2) {
          finger = finger.translate([37, -47]);
        } else if (digit === 3) {
          finger = finger.translate([39, -1]);
        } else if (digit === 4) {
          finger = finger.translate([76, -47]);
        }
        if (!left) {
          finger = finger.rotateZ(180);
          finger = finger.translate([-1, thumbBounds[1].y - params.buttonSide]);
        }
      }
      world.push(finger);
    }
  }
  return world;
}
