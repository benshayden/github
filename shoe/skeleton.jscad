/*jslint sloppy: true, vars: true, indent: 2 */
var PARAMS = [{'name': 'resolution', 'caption': 'Resolution:', 'type': 'int', 'default': 24}];
function circle(center, radius, start, end, maxedge) {
  var d_angle = 2 * Math.asin(maxedge / (2 * radius));
}
function main() {
  return [linear_extrude({height: 10},
    polygon([[0,0],[10,10],[10,0]]))];
}
