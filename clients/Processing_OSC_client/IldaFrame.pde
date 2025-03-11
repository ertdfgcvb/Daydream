class IldaFrame {

  int blankCount; // how many blank points to send at path ends
  int endCount;   // how many end repeats to send
  int startCount;

  boolean doFlipX;
  boolean doFlipY;

  final static int MIN_POINT     = -32768;
  final static int MAX_POINT     =  32767;
  final static int MAX_INTENSITY =  255;
  final color BLACK = color(0);

  ArrayList<IldaPoint> points;

  PVector source_min, source_max;

  IldaFrame() {
    blankCount = 10;
    startCount = 0;
    endCount   = 10;
    doFlipX    = false;
    doFlipY    = false;
    source_min = new PVector(-1, 1);
    source_max = new PVector(-1, 1);
    points = new ArrayList();
  }

  void clear() {
    points.clear();
  }

  IldaPoint processPoint(PVector pos, color col) {

    IldaPoint out = new IldaPoint();

    out.x = (int) map(constrain(pos.x, source_min.x, source_max.x), -source_min.x, source_max.x, MIN_POINT, MAX_POINT);
    out.y = (int) map(constrain(pos.y, source_min.y, source_max.y), -source_min.y, source_max.y, MIN_POINT, MAX_POINT);
    out.r = (int) (red(col)   / 255.0 * MAX_INTENSITY);
    out.g = (int) (green(col) / 255.0 * MAX_INTENSITY);
    out.b = (int) (blue(col)  / 255.0 * MAX_INTENSITY);

    if (doFlipX) out.x = -out.x;
    if (doFlipY) out.y = -out.y;

    return out;
  }

  void processPolyline(Polyline p) {
    if (p.size() > 0) {

      for (int i=0; i<blankCount; i++) {
        points.add(processPoint(p.first().pos, BLACK));
      }

      // repeat at start
      for (int i=0; i<startCount; i++) {
        points.add(processPoint(p.first().pos, p.first().col));
      }

      // add points
      for (int i=0; i<p.size(); i++) {
        points.add(processPoint(p.get(i).pos, p.get(i).col));
      }

      // repeat at end
      for (int n=0; n<endCount; n++) {
        points.add(processPoint(p.last().pos, p.last().col));
      }

      // blanking at end
      for (int i=0; i<blankCount; i++) {
        points.add(processPoint(p.last().pos, BLACK));
      }
    }
  }
}
