// Send an ILDA frame via OSC to the Ether Dream OSC server.

import oscP5.*;
import netP5.*;

OscP5 oscP5;
NetAddress dac;

IldaFrame frame;

// Points per second
int pps = 30000;
boolean send = true;

void setup() {
  size(700, 700, P3D);
  frameRate(30);
  textFont(loadFont("Monospaced-14.vlw"));

  /* start oscP5, listening for incoming messages at port 9001 */
  oscP5 = new OscP5(this, 9001);
  dac = new NetAddress("localhost", 9000);

  frame = new IldaFrame();

  // Map the frame to the canvas size / coords
  frame.source_max.x = width;
  frame.source_max.y = height;
  frame.source_min.x = 0;
  frame.source_min.y = 0;

  // Flip X, Y?
  frame.doFlipX = false;
  frame.doFlipY = false;

  // Blank count: how many empty points to send before each line
  frame.blankCount = 0;
  // Start count: how many repeated points at the beginning of a line
  frame.startCount = 0;
  // Start count: how many repeated points at the end of a line
  frame.endCount = 0;

  // Set up the server
  oscP5.send(new OscMessage("/set_pps").add(pps), dac);
  oscP5.send(new OscMessage("/set_loop_count").add(-1), dac);
  oscP5.send(new OscMessage("/set_wait_for_ready").add(0), dac);

  // Draw a test pattern (on server)
  // oscP5.send(new OscMessage("/test_pattern").add(pps), dac);
  // exit(); // quit if test
}

void draw() {

  // Add a Lissajus curve
  Polyline shape = new Polyline();

  float t = frameCount * 0.02;
  int res = 256;  
  for (int i=0; i<res; i++) {
    float a = TWO_PI / res * i;
    float r = sin( a * 16 - t * 8 ) * 0.3;
    float x = width/2 + (cos(a * 3 + t) * (1 - r)) * width * 0.1;
    float y = height/2 + (sin(a * 6) * (1 - r)) * height * 0.1;
    float R = floor(map(sin(a * 2 - t * 6), -1, 1, 0, 255));
    float G = floor(map(sin(a * 3 - t * 7), -1, 1, 0, 255));
    float B = floor(map(sin(a * 4 - t * 8), -1, 1, 0, 255));

    shape.add(x, y, R, G, B);
  }

  shape.closed = true;

  // Clear the frame!
  frame.clear();

  // Add the shape(s)
  frame.processPolyline(shape);
  
  // Send OSC bundle
  sendFrameBundle(frame);

  // Preview output
  noFill();
  background(0);
  stroke(255);

  drawPolyline(g, shape, true);

  // Display some stats
  String out = "";

  out += "FPS: " + round(frameRate) + "\n";
  out += "send: " + send + "\n";
  out += "num points (f): " + frame.points.size() + "\n";
  out += "pps: " + pps + "\n";

  fill(255);
  text(out, width - 200, 40);
}

void sendFrame(IldaFrame frame) {
  oscP5.send(new OscMessage("/clear"), dac);
  OscMessage p = new OscMessage("/p");
  for (IldaPoint pt : frame.points) {
    p.clear();
    p.setAddrPattern("/p");
    p.add(pt.toArray());
    oscP5.send(p, dac);
  }
}

void sendFrameBundle(IldaFrame frame) {
  OscBundle bundle = new OscBundle();

  OscMessage m = new OscMessage("/clear");
  bundle.add(m);

  for (IldaPoint pt : frame.points) {
    m.clear();
    m.setAddrPattern("/c");
    m.add(pt.RGBtoArray());
    bundle.add(m);

    m.clear();
    m.setAddrPattern("/x");
    m.add(pt.XYtoArray());
    bundle.add(m);
  }

  oscP5.send(bundle, dac);
}

Polyline steppedSegment(float x1, float y1, float x2, float y2, float step, color stroke) {

  Polyline p = new Polyline();

  float d = dist(x1, y1, x2, y2);

  if (d < step) {
    p.add(x1, y1, stroke);
    p.add(x2, y2, stroke);
  } else {
    int num = floor(d / step);
    float real_step = d / num;
    float dx = (x2 - x1) / d * real_step;
    float dy = (y2 - y1) / d * real_step;
    for (int i=0; i<num; i++) {
      float x = x1 + i * dx;
      float y = y1 + i * dy;
      p.add(x, y, stroke);
    }
    p.add(x2, y2, stroke);
  }

  return p;
}
