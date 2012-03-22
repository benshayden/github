/*jslint sloppy: true, vars: true, indent: 2, bitwise: true */
var PARAMS = [{name: 'radius', caption: 'radius:', type: 'float', 'default': 10},
  {name: 'maxEdgeLength', caption: 'max edge length:', type: 'float', 'default': 1}];
var BG = [1, 1, 1, 1];
var LIGHT = [-0.25, -0.25, 0.9];
function flatten(xs) { return xs.length ? xs.reduce(function (a, b) { return a.concat(b); }) : []; }
var colors = [[1, 0, 0], [0, 1, 0], [0, 0, 1], [1, 1, 0]];
function geodesic(center, radius, maxEdgeLength) {
  var mybasis = (new CSG.Vector2D((1 + Math.sqrt(5)), 2)).times(radius / Math.sqrt(10 + (2 * Math.sqrt(5)))), vertices = [], adjacency = 0, tri = [], path = [8, 10, 3, 9, 11, 5, 6, 4, 10, 8, 0, 6, 1, 5, 7, 2, 0, 8, 3, 7, 9, 5];
  vertices = flatten([0, 1, 2, 3].map(function (signs) {
    var sa = mybasis.x * ((signs & 1) ? -1 : 1), sb = mybasis.y * ((signs & 2) ? -1 : 1);
    return [[0, sb, sa], [sa, 0, sb], [sb, sa, 0]].map(function (cs) { return center.plus(new CSG.Vector3D(cs)); });
  }));
  function sphericalTriangle(abc, r) {
    if (abc.map(function (v, i) { return v.minus(abc[(i + 1) % 3]).length() < maxEdgeLength; }).reduce(function (x, y) { return x && y; })) {
      return [new CSG.Polygon(abc.map(function (v) { return new CSG.Vertex(v, v); }), new CSG.Polygon.Shared(colors[r % colors.length]))];
    }
    var ms = abc.map(function (v, i) {
      return v.plus(abc[(i + 1) % 3]).dividedBy(2).minus(center).unit().times(radius).plus(center);
    });
    return sphericalTriangle(ms, (r + 1)).concat(flatten(abc.map(function (v, i) {
      return sphericalTriangle([ms[(i + 2) % 3], v, ms[i]], (r + i + 2));
    })));
  }
  tri = path.slice(0, 3).map(function (i) { return vertices[i]; });
  adjacency = (vertices[0].minus(vertices[1]).length() + vertices[5].minus(vertices[0]).length()) / 2;
  return CSG.fromPolygons(sphericalTriangle(tri, 0).concat(flatten(path.slice(3).map(function (vi, ii) {
    tri.splice(((adjacency < tri[0].minus(vertices[vi]).length()) ? 0 : ((adjacency < tri[1].minus(vertices[vi]).length()) ? 1 : 2)), 1);
    tri.splice(1, 0, vertices[vi]);
    return sphericalTriangle(tri, ii + 1);
  }))));
}
function main(params) {
  return geodesic(new CSG.Vector3D(0, 0, 0), params.radius, params.maxEdgeLength);
}