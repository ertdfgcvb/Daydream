
// A screen point 
class Point {
  PVector pos;
  color col;

  Point(float x, float y, color col) {
    this.pos = new PVector(x, y);
    this.col = col;
  }
}


class Polyline {

  ArrayList<Point> points;
  boolean closed = false;

  Polyline() {
    points = new ArrayList();
  }

  void add(float x, float y, float r, float g, float b) {
    points.add(new Point(x, y, color(r, g, b)));
  }

  void add(float x, float y, color col) {
    points.add(new Point(x, y, col));
  }

  Point first() {
    return points.get(0);
  }

  Point last() {
    return points.get(points.size()-1);
  }

  Point get(int i) {
    return points.get(i);
  }

  void clear() {
    points.clear();
  }

  int size() {
    return points.size();
  }
}

void drawPolyline(PGraphics g, Polyline p, boolean dotted) {
  g.noFill();
  
  // Note: PShape has a bug and won't draw shapes with point cound < 3!
  g.beginShape();
  for (Point v : p.points) {
    g.stroke(v.col);
    g.vertex(v.pos.x, v.pos.y);
  }
  g.endShape(p.closed ? CLOSE : 0);

  if (dotted) {
    g.noStroke();
    for (Point v : p.points) {
      g.fill(v.col);
      g.rect(v.pos.x-2, v.pos.y-2, 4, 4);
    }
  }
}
