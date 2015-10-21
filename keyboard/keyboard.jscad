'use strict';
// Everything is in mm.

var cylinder, cube, sphere, linear_extrude, union, polygon;

function getParameterDefinitions() {
  var params = [];
  params.push({name: 'displayMode', type: 'choice', values: ['visual', 'printable']});
  params.push({name: 'baseBones', type: 'float', initial: 13});
  params.push({name: 'baseWidth', type: 'float', initial: 50});
  params.push({name: 'boneDiameter', type: 'float', initial: 3.5});
  params.push({name: 'buttonSide', type: 'float', initial: 8});
  params.push({name: 'color', type: 'text', initial: '667fe5'});
  params.push({name: 'boneColor', type: 'text', initial: 'ffffff'});
  var parts = ['all', 'base'];
  ['left', 'right'].forEach(function(hand) {
    parts.push(hand + 'Thumb');
    params.push({name: hand + 'ThumbHeight', type: 'float', initial: 20});
    params.push({name: hand + 'ThumbFrontWidth', type: 'float', initial: 5});
    params.push({name: hand + 'ThumbBackWidth', type: 'float', initial: 7});
    for (var digit = 1; digit < 5; ++digit) {
      parts.push(hand + digit);
      params.push({name: hand + digit + 'FrontHeight', type: 'float', initial: 4});
      params.push({name: hand + digit + 'BackHeight', type: 'float', initial: 7});
      params.push({name: hand + digit + 'FrontWidth', type: 'float', initial: 4});
      params.push({name: hand + digit + 'BackWidth', type: 'float', initial: 6});
    }
  });
  params.unshift({name: 'part', type: 'choice', values: parts});
  return params;
}

function parseColor(color) {
  var parsed = [];
  for (var i = 0; i < 3; ++i) {
    parsed.push(parseInt(color.slice(i * 2, (i + 1) * 2), 16) / 255);
  }
  return parsed;
}

function makeDome(buttonSide) {
   var c0 = cylinder({r: buttonSide / 2, h: buttonSide});
   var c1 = c0.rotateX(90).translate([0, buttonSide / 2, buttonSide / 2]);
   var q = cube({size:[buttonSide, buttonSide, buttonSide]}).translate([0, -buttonSide / 2, 0]);
   var dome = c0.intersect(c1).subtract(q);
   dome = dome.center().translate([-buttonSide / 4, buttonSide / 2, buttonSide / 2]);
   dome = dome.rotateZ(-90);
   return dome;
}

function makeWire(buttonSide) {
  var wire = [[0.5, 0], [0, 0.5], [0, 2], [0.5, 2.5], [1, 2], [1, 0.5]];
  wire = linear_extrude({height: buttonSide}, polygon(wire)).rotateX(90).translate([0,buttonSide, -0.5]);
  wire = union(wire, wire.translate([0, 0, buttonSide - 1.5]));
  return wire;
}

function makeCorner(buttonSide) {
  var corner = cylinder({r: buttonSide, h: buttonSide});
  corner = corner.intersect(cube({size:[buttonSide, buttonSide, buttonSide]}));
  return corner;
}

function makeBone(boneDiameter, len) {
  return cylinder({r: boneDiameter / 2, h: len || 1});
}

function makeBend(boneDiameter) {
  return sphere({r: boneDiameter / 2, center: true});
}

function makeThumb(height, frontWidth, backWidth, buttonSide, wire, corner, bone) {
  var width = frontWidth + backWidth;
  var thumb = [];
  
  var piece = cube({size: [buttonSide, buttonSide + height, buttonSide]});
  piece = piece.subtract(wire.rotateZ(-90).translate([0, (buttonSide + height) / 2, 0]));
  piece = piece.translate([0, buttonSide, 0]);
  thumb.push(piece);
  
  piece = piece.translate([buttonSide + width + buttonSide, 0]);
  piece = piece.subtract(piece.translate([2, 0]));
  thumb.push(piece);

  piece = cube({size: [buttonSide + width, buttonSide, buttonSide]});
  piece = piece.subtract(wire.translate([(buttonSide / 2) + frontWidth, 0, 0]));
  piece = piece.translate([buttonSide, 0, 0]);
  thumb.push(piece);
  
  piece = piece.translate([0, buttonSide + height + buttonSide]);
  thumb.push(piece);
  
  piece = corner.rotateZ(180);
  piece = piece.translate([buttonSide, buttonSide, 0]);
  thumb.push(piece);
  
  piece = corner.rotateZ(90);
  piece = piece.translate([buttonSide, buttonSide + height + buttonSide]);
  thumb.push(piece);
  
  piece = corner.scale([1 / 4, 1, 1]);
  piece = piece.translate([buttonSide + width + buttonSide, buttonSide + height + buttonSide]);
  thumb.push(piece);
  
  piece = corner.rotateZ(-90);
  piece = piece.scale([1 / 4, 1, 1]);
  piece = piece.translate([buttonSide + width + buttonSide, buttonSide]);
  thumb.push(piece);

  thumb = union(thumb);
  
  piece = bone.scale([1, 1, buttonSide + height + buttonSide + (buttonSide / 2)]);
  piece = piece.rotateX(-90);
  piece = piece.translate([0, 0, buttonSide / 2]);
  thumb = thumb.subtract(piece);

  piece = bone.scale([1, 1, buttonSide + width]);
  piece = piece.rotateY(90);
  piece = piece.translate([0, height + (2.5 * buttonSide), buttonSide / 2]);
  thumb = thumb.subtract(piece);

  return thumb;
}

function getBaseLength(baseBones, boneDiameter) {
  return (baseBones + 0.5) * 2 * boneDiameter;
}

function makeBase(baseWidth, baseBones, boneDiameter, bone) {
  var baseSide = baseWidth / Math.sqrt(2);
  var baseLength = getBaseLength(baseBones, boneDiameter);
  var base = cube({size: [baseLength, baseSide, baseSide]});
  base = base.rotateX(-45);
  base = base.intersect(cube({size: [baseLength, baseWidth, baseSide]}));
  var piece = bone.scale([1, 1, baseLength]);
  piece = piece.rotateY(90);
  piece = piece.translate([0, (baseWidth / 2), 2.5 * boneDiameter]);
  base = base.subtract(piece);
  
  piece = cube({size: [boneDiameter, (baseWidth / 2) + (1.5 * boneDiameter), baseSide]});
  piece = piece.translate([boneDiameter, 0, boneDiameter]);
  piece = piece.union(piece.translate([boneDiameter * 2, (baseWidth / 2) - (1.5 * boneDiameter), 0]));
  for (var i = 0; i < Math.ceil(baseBones / 2); ++i) {
    base = base.subtract(piece);
    piece = piece.translate([boneDiameter * 4, 0, 0]);
  }
  
  piece = cube({size: [boneDiameter, baseWidth / 2, boneDiameter]});
  piece = piece.translate([0, baseWidth / 2, 2 * boneDiameter]);
  base = base.subtract(piece);
  
  piece = piece.translate([baseLength - boneDiameter, 0, 0]);
  if ((baseBones % 2) === 0) {
      piece = piece.translate([0, -baseWidth / 2, 0]);
  }
  base = base.subtract(piece);

  return base;
}

function makeBaseBones(baseWidth, baseBones, boneDiameter, base, bone, bend) {
  var baseLength = getBaseLength(baseBones, boneDiameter);
  var baseBonePieces = [];

  // the long piece all the way through
  var piece = bone.scale([1, 1, baseLength - boneDiameter]);
  piece = piece.rotateY(90);
  piece = piece.translate([boneDiameter / 2, baseWidth / 2, 2.5 * boneDiameter]);
  baseBonePieces.push(piece);
  
  // the shorter piece on the side
  piece = piece.scale([(baseWidth / 3) / baseLength, 1, 1]);
  piece = piece.rotateZ(90);
  piece = piece.translate([(baseWidth / 2) + (boneDiameter / 2), baseWidth / 2, 0]);
  baseBonePieces.push(piece);
  
  // the shorter piece on the other side
  piece = piece.translate([baseLength - boneDiameter, 0, 0]);
  if ((baseBones % 2) === 0) {
    piece = piece.translate([0, -baseWidth / 3, 0]);
  }
  baseBonePieces.push(piece);
  
  piece = bend.translate([boneDiameter / 2, baseWidth / 2, 2.5 * boneDiameter]);
  baseBonePieces.push(piece);

  piece = piece.translate([baseLength - boneDiameter, 0, 0]);
  baseBonePieces.push(piece);
  
  piece = bone.scale([1, 1, (baseWidth / 2) + (1.5 * boneDiameter)]);
  piece = piece.rotateX(-90);
  piece = piece.translate([(1.5 * boneDiameter), 0, 1.5 * boneDiameter]);
  piece = piece.union(piece.rotateX(90).translate([0, (2.5 * boneDiameter) + (baseWidth / 2), boneDiameter]));
  piece = piece.union(piece.rotateX(-90).translate([2 * boneDiameter, (baseWidth / 2) - (2.5 * boneDiameter), (2.5 * boneDiameter) + (baseWidth / 2)]));
  piece = piece.intersect(cube({size: [boneDiameter * 6, baseWidth, baseWidth / 2]}));
  for (var i = 0; i < Math.ceil(baseBones / 2); ++i) {
    baseBonePieces.push(piece);
    piece = piece.translate([boneDiameter * 4, 0, 0]);
  }

  baseBonePieces = union(baseBonePieces);
  baseBonePieces = baseBonePieces.intersect(cube({size: [baseLength, baseWidth, baseWidth / 2]}));
  return baseBonePieces;
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

function makeFinger(frontHeight, backHeight, frontWidth, backWidth, buttonSide, wire, dome, corner, bone) {
  var finger = [];
  
  var piece = cube({size: [buttonSide, buttonSide + frontHeight, buttonSide]});
  piece = piece.translate([0, buttonSide, 0]);
  piece = piece.subtract(wire.rotateZ(-90).translate([0, (1.5 * buttonSide) + frontHeight]));
  finger.push(piece);
  
  piece = corner.rotateZ(180);
  piece = piece.translate([buttonSide, buttonSide]);
  finger.push(piece);
  
  piece = dome.translate([0, 2 * buttonSide + frontHeight]);
  finger.push(piece);
  
  piece = cube({size: [frontWidth + buttonSide + backWidth, buttonSide, buttonSide]}).translate([0, 0, 0]);
  piece = piece.translate([buttonSide, 0]);
  piece = piece.subtract(wire.translate([frontWidth + (1.5 * buttonSide), 0]));
  finger.push(piece);
  
  piece = corner.rotateZ(-90);
  piece = piece.translate([buttonSide + frontWidth + buttonSide + backWidth, buttonSide]);
  finger.push(piece);
  
  var backX = buttonSide + frontWidth + buttonSide + backWidth;
  piece = cube({size: [buttonSide, buttonSide + backHeight, buttonSide]});
  piece = piece.translate([backX, buttonSide]);
  piece = piece.subtract(wire.rotateZ(-90).translate([backX, (1.5 * buttonSide) + backHeight]));
  finger.push(piece);
  
  piece = dome.translate([backX, 2 * buttonSide + backHeight]);
  finger.push(piece);

  finger = union(finger);
  
  piece = bone.scale([1, 1, 2 * buttonSide + backHeight]);
  piece = piece.rotateX(-90);
  piece = piece.translate([backX + buttonSide / 2, 0, buttonSide / 2]);
  finger = finger.subtract(piece);
  
  piece = bone.scale([1, 1, backX + buttonSide / 2]);
  piece = piece.rotateY(90);
  piece = piece.translate([0, 0, buttonSide / 2]);
  finger = finger.subtract(piece);
  
  return finger;
}

function makeThumbButtons(height, frontWidth, backWidth, buttonSide, thumb, button) {
  var piece = button.rotateX(-90).rotateY(90);
  piece = piece.translate([2 * buttonSide + frontWidth + 0.7, buttonSide, buttonSide]);
  thumb = thumb.union(piece);
  
  piece = button.rotateY(90);
  piece = piece.translate([buttonSide, buttonSide + height / 2 - 0.7, buttonSide]);
  thumb = thumb.union(piece);
  
  piece = button.rotateX(90).rotateY(-90);
  piece = piece.translate([2 * buttonSide + frontWidth + 0.7, 2 * buttonSide + height]);
  thumb = thumb.union(piece);
  
  piece = cube({size: [buttonSide, buttonSide, buttonSide]});
  piece = piece.translate([0, 0, -2]);
  piece = button.intersect(piece);
  piece = piece.rotateY(-90);
  piece = piece.translate([2 * buttonSide + frontWidth + backWidth, buttonSide + height / 2 - 0.7]);
  thumb = thumb.union(piece);
  return thumb;
}

function makeFingerButtons(frontHeight, backHeight, frontWidth, backWidth, buttonSide, finger, button) {
  var piece = button.rotateX(-90).rotateY(-90);
  piece = piece.translate([buttonSide + frontWidth, buttonSide]);
  finger = finger.union(piece);
  
  piece = button.rotateY(90).rotateX(180);
  piece = piece.translate([buttonSide, 2 * buttonSide + frontHeight]);
  finger = finger.union(piece);
  
  piece = button.rotateY(-90);
  piece = piece.translate([2 * buttonSide + frontWidth + backWidth, buttonSide + backHeight - 0.7]);
  finger = finger.union(piece);
  return finger;
}

function makeFingerBones(fingerWidth, buttonSide, boneDiameter, bone, bend) {
  var piece = bone.rotateY(90);
  piece = piece.scale([(buttonSide * 1.5) + fingerWidth, 1, 1]);
  piece = piece.translate([buttonSide, 0, buttonSide / 2]);
  piece = piece.union(piece.rotateZ(90).translate([fingerWidth + (buttonSide * 2.5), -buttonSide, 0]));
  piece = piece.union(bend.translate([fingerWidth + (2.5 * buttonSide), 0, buttonSide / 2]));
  return piece;
}

function makeThumbBones(buttonSide, boneDiameter, thumb, bone, bend) {
  var height = thumb.getBounds()[1].y;
  var thumbBones = [];
  var piece = bone.scale([1, 1, height - (buttonSide / 2)]);
  piece = piece.rotateX(-90);
  piece = piece.translate([0, 0, buttonSide / 2]);
  thumbBones.push(piece);
  piece = bone.scale([1, 1, 2 * buttonSide]);
  piece = piece.rotateY(90);
  piece = piece.translate([0, height - (buttonSide / 2), buttonSide / 2]);
  thumbBones.push(piece);
  piece = bend.translate([0, height - (buttonSide / 2), buttonSide / 2]);
  thumbBones.push(piece);
  return union(thumbBones);
}

function rad2deg(rad) {
    return rad * 180 / Math.PI;
}

function makeConnectionBone(start, end, boneDiameter, bone, bend) {
  var boneLength = Math.sqrt(
      Math.pow(end[0] - start[0], 2) +
      Math.pow(end[1] - start[1], 2) +
      Math.pow(end[2] - start[2], 2));
  bone = bone.scale([1, 1, boneLength]);
  bone = bone.rotateY(rad2deg(Math.acos((end[2] - start[2]) / boneLength)));
  var zAngle = 0;
  if (start[0] === end[0]) {
    bone = bone.rotateZ(Math.sign(end[1] - start[1]) * 90);
  } else {
    bone = bone.rotateZ(rad2deg(Math.atan2((end[1] - start[1]), (end[0] - start[0]))));
  }
  bone = bone.translate(start);
  bone = bone.union(bend.translate(start));
  bone = bone.union(bend.translate(end));
  return bone;
}

function makeThumbConnectionBone(left, buttonSide, boneDiameter, base, thumb, bone, bend) {
  var thumbBounds = thumb.getBounds();
  var baseBounds = base.getBounds();
  var thumbBone = [
      thumbBounds[left ? 1 : 0].x + (0.5 * boneDiameter * (left ? -1 : 1)),
      thumbBounds[1].y - (buttonSide / 2),
      thumbBounds[0].z
  ];
  var baseBone = [
      baseBounds[left ? 1 : 0].x + (1.5 * boneDiameter * (left ? -1 : 1)),
      baseBounds[0].y + (baseBounds[1].y / 2) + boneDiameter * (left ? -1 : 1),
      baseBounds[1].z
  ];
  return makeConnectionBone(baseBone, thumbBone, boneDiameter, bone, bend);
}

function makeFingerConnectionBone(left, digit, buttonSide, boneDiameter, baseBones, base, finger, bone, bend) {
  var fingerBounds = finger.getBounds();
  var baseBounds = base.getBounds();
  var fingerBone = [
      fingerBounds[left ? 1 : 0].x + (buttonSide * (left ? -1 : 1) / 2),
      fingerBounds[1].y - buttonSide,
      fingerBounds[0].z + (boneDiameter / 2)
  ];
  var baseBone = [
      baseBounds[left ? 0 : 1].x + (((4 * boneDiameter * (4 - digit)) + (1.5 * boneDiameter)) * (left ? 1 : -1)),
      baseBounds[0].y + (baseBounds[1].y / 2) + boneDiameter * (left ? -1 : 1) * (((baseBones % 2) === 0) ? -1 : 1),
      baseBounds[1].z
  ];
  return makeConnectionBone(baseBone, fingerBone, boneDiameter, bone, bend);
}

function main(params) {
  params.color = parseColor(params.color);
  params.boneColor = parseColor(params.boneColor);
  var wire = makeWire(params.buttonSide);
  var dome = makeDome(params.buttonSide);
  var corner = makeCorner(params.buttonSide);
  var bone = makeBone(params.boneDiameter);
  var bend = makeBend(params.boneDiameter);
  var button = makeButton(params.buttonSide);
  var base = makeBase(params.baseWidth, params.baseBones, params.boneDiameter, bone);
  base = base.setColor(params.color);
  if (params.part === 'base') {
    return base;
  }
  var baseBounds = base.getBounds();

  var world = [];

  for (var left = 0; left < 2; ++left) {
    var hand = left ? 'left' : 'right';
    var thumbHeight = params[hand + 'ThumbHeight'];
    var thumbFrontWidth = params[hand + 'ThumbFrontWidth'];
    var thumbBackWidth = params[hand + 'ThumbBackWidth'];
    var thumb = makeThumb(thumbHeight, thumbFrontWidth, thumbBackWidth, params.buttonSide, wire, corner, bone);
    thumb = thumb.setColor(params.color);
    if (params.part === 'thumb') {
      return thumb;
    }
    var thumbBounds = thumb.getBounds();
    var handBase = base;

    if (params.displayMode === 'visual') {
      thumb = makeThumbButtons(thumbHeight, thumbFrontWidth, thumbBackWidth, params.buttonSide, thumb, button);
      handBase = handBase.union(makeBaseBones(params.baseWidth, params.baseBones, params.boneDiameter, handBase, bone, bend).setColor(params.boneColor));
      thumb = thumb.union(makeThumbBones(params.buttonSide, params.boneDiameter, thumb, bone, bend).setColor(params.boneColor));
      thumb = thumb.rotateX(90);
      
      if (left) {
        thumb = thumb.rotateZ(180);
        handBase = handBase.rotateZ(180);
        thumb = thumb.translate([-params.buttonSide, params.buttonSide, params.buttonSide + (params.baseWidth / 2)]);
        handBase = handBase.translate([-params.buttonSide, params.baseWidth]);
      } else {
        thumb = thumb.translate([params.buttonSide, 2 * params.buttonSide, params.buttonSide + (params.baseWidth / 2)]);
        handBase = handBase.translate([params.buttonSide, 0]);
      }
      world.push(makeThumbConnectionBone(left, params.buttonSide, params.boneDiameter, handBase, thumb, bone, bend).setColor(params.boneColor));
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
      finger = finger.setColor(params.color);
      if (params.part === 'finger') {
        return finger;
      }
      var fingerBounds = finger.getBounds();

      if (params.displayMode === 'visual') {
        finger = makeFingerButtons(frontHeight, backHeight, frontWidth, backWidth, params.buttonSide, finger, button);
        finger = finger.union(makeFingerBones(frontWidth + backWidth, params.buttonSide, params.boneDiameter, bone, bend).setColor(params.boneColor));
        finger = finger.rotateX(90).rotateZ(-90);
        finger = finger.translate([
              (left ? -1 : 1) * (thumbBounds[1].x + params.buttonSide * (2 * digit + !left)),
              fingerBounds[1].x - fingerBounds[0].x,
              params.buttonSide + (params.baseWidth / 2)]);
        world.push(makeFingerConnectionBone(left, digit, params.buttonSide, params.boneDiameter, params.baseBones, handBase, finger, bone, bend).setColor(params.boneColor));
      } else {
        finger = finger.rotateZ(((digit % 2) ? -1 : 1) * 90);
        if (digit === 1) {
          finger = finger.translate([0, -10]);
        } else if (digit === 2) {
          finger = finger.translate([3.5 * params.buttonSide + frontHeight + 1, -1 - fingerBounds[1].x]);
        } else if (digit === 3) {
          finger = finger.translate([3.5 * params.buttonSide + frontHeight + 2, -10]);
        } else if (digit === 4) {
          finger = finger.translate([8 * params.buttonSide + frontHeight, -1 - fingerBounds[1].x]);
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
