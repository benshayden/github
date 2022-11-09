A Parametric Signed Distance Field, PSDF, is an object that can be
 - configured in a GUI with gizmos like womp3d.com,
 - edited as text,
 - compiled to an SDF pixel shader fragment,
 - ray-marched in your pixel shader,
 - generate a trimesh using marching cubes.

```
parameters:
  # cannot include point, time, distance, materialWeights, other glsl builtins
  # can include any builtin glsl type or structs
  # set to specific values for preview
  a: 1.0 # indicates that 'a' is a float that a preview can set to 1.0
  tex: grid.png # 'tex' is a texture that a preview can set to grid.png in the same directory as this file
materials:
  metal:
    metalness: 1.0
  glass:
    transparency: 1.0
  bark:
    triplanar:
      ...
# all fields other than parameters, materials are compiled to psdf functions as unions.
Robot:
  order: 5 # default=3. https://iquilezles.org/articles/smoothsteps/
  factor: a * a # values can be glsl expressions referencing parameters
  radius: a / 2
  parts:
  - shape: sphere
    radius: vec3(1, 2, 4) # float|vec3
    rotation: vec3(0, 0.1, 0.2) # euler turns XYZ
    position: vec3(1, 2, 3)
    material: metal
  - shape: union
    order: 4
    factor: 0.1
    radius: 1 # rounding
    parts:
    - shape: cylinder
      topRadius: 1
      bottomRadius: 2
      height: 3
      position: [2, 2, 2] # vec3
      rotation: [0.1, 0.2, 0] # euler turns XYZ
      material: bark
    - shape: box
      size: 2 # float|vec3
      position:
      rotation:
      material: glass
  - shape: complement # like union but subtracted from parent
    order:
    factor:
    radius:
    parts:
    - shape: intersection # like union but exclusive
      order:
      factor:
      radius:
      parts:
      -
```

Shapes: union, intersection, complement, sphere, box, cylinder, curve.

The PSDF compiler should produce GLSL like this:

```
void psdfRobot(
    vec3 point, float time,
    float a, texture tex,
    out float distance, out float materialWeights[5]) {
}
```
