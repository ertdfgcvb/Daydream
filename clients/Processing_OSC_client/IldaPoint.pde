class IldaPoint {
  int x, y;    // 16 bits
  int r, g, b; // 8 bits
  
  IldaPoint(){
    
  }
  
  IldaPoint(int x, int y){
    this.x = x;
    this.y = y;
    this.r = 255;
    this.g = 255;
    this.b = 255;
  }
  
  IldaPoint(int x, int y, int r, int g, int b ){
    this.x = x;
    this.y = y;
    this.r = r;
    this.g = g;
    this.b = b;
  }  
  
  int[] toArray(){
    return new int[]{x, y, r, g, b};
  }
  
  int[] XYtoArray(){
    return new int[]{x, y};
  }
  
  int[] RGBtoArray(){
    return new int[]{r, g, b};
  }
}
