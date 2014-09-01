//
//  lineextract.hpp
//  LineExtraction
//
//  Created by cristi on 8/31/14.
//  Copyright (c) 2014 cristi. All rights reserved.
//

#ifndef LineExtraction_lineextract_hpp
#define LineExtraction_lineextract_hpp

#include "Common.hpp"

using namespace cv;
using namespace std;

struct _point2d;
typedef _point2d t_point2d;

struct _region;
typedef _region t_region;

struct _line2d;
typedef _line2d t_line2d;

typedef vector<t_line2d>    t_lineVector;
//typedef vector<t_houghLine> t_houghLineVector;
typedef vector<t_point2d>   t_pointVector;
typedef unsigned char t_uchar;

struct _region {
  size_t  row;
  size_t  column;
};

struct _point2d : public Eigen::Vector2f{
public:
  friend ostream& operator<<(ostream& , t_point2d& );
  
  explicit _point2d(const float_t x,
                    const float_t y){
    this->x() = x;
    this->y() = y;
  }
  _point2d( const _point2d& p ): Eigen::Vector2f(p){}
  _point2d() = default;
  
  bool operator==(const t_point2d& other) const{
    return (x() == other.x() && y() == other.y());
  }
  
  float_t slopeBetween( const t_point2d& other ) const{
    //return (*KERNEL_DISTANCES(y(),x()))(other.y(),other.x());
    return ((other.y()-y())/(other.x()-x()));
  }
  
  float_t lengthBetween( const t_point2d& p ) const{
    //return (*KERNEL_LENGTHS(y(),x()))(p.y(),p.x());
    float_t x = p.x() - this->x();
    float_t y = p.y() - this->y();
    x = powf(x,2.0f);
    y = powf(y,2.0f);
    x+=y;
    return sqrtf(x);
  }
  
  t_point2d centerBetween( const t_point2d& p) const{
    return t_point2d{
      ((this->x()+p.x())*0.5f),
      ((this->y()+p.y())*0.5f)
    };
  }
  
  friend ostream& operator<<(ostream& os, t_point2d& p);
};

ostream& operator<<(ostream& os, t_point2d& p){
  os << "(" << p.x() << "," << p.y() << ")";
  return os;
}

struct _line2d {
  t_point2d   p1;
  t_point2d   p2;
  t_region    region;
  float_t     slope;
  bool        normalized;
  
  float_t length() const{
    return p1.lengthBetween(p2);
  }
  
  explicit _line2d(const t_point2d& p1,
                   const t_point2d& p2):
  p1(p1),
  p2(p2){}
  
  explicit _line2d(const t_point2d& p1,
                   const t_point2d& p2,
                   const t_region& r,
                   float_t slope,
                   float_t intercept):
  p1(p1),
  p2(p2),
  region(r),
  slope(slope){
    // this->slope = (*KERNEL_DISTANCES(p1.y(),p1.x()))(p2.y(),p2.x());
  }
  
  _line2d() = default;
  
  t_point2d centerPoint() const{
    return p1.centerBetween(p2);
  }
  /// p1 should always be on the left of the line or in the upper part
  void normalize(){
    if (normalized) return;
    if (p1.x() > p2.x()){
      t_point2d p = p1;
      p1 = p2;
      p2 = p1;
    }
    if (p1.x() == p2.x() && p1.y() < p2.y()){
      t_point2d p = p1;
      p1 = p2;
      p2 = p1;
    }
  }
  
  bool operator==(const _line2d& l) const{
    return false;
  }
};

ostream& operator<<(ostream& os, t_line2d& l){
  os << " SLOPE " << l.slope << " P1 " << l.p1 << " P2 " << l.p2 << endl;
  return os;
}

float computeLength( size_t x1, size_t y1, size_t x2, size_t y2 ){
  Eigen::Vector2f p1(x1,y1);
  Eigen::Vector2f p2(x2,y2);
  return (p1-p2).norm();
}

class random_lines {
public:
  void generate(const size_t threads,
                const size_t rows,
                const size_t columns){
    _line_matrix.resize(rows, columns);
    
    if (threads == 1){
      processRows();
    }else{
      _random_pp.start(threads);
    }
  }
  
  
  bool toFile(string filepath){
    
    ofstream file(filepath);
    
    if (!file.is_open())
      return false;
    
    t_lineVector lines;
    
    if (!toVector(lines))
      return false;
    
    file << "PRINTING " <<  lines.size() << " RANDOM GENERATED LINES" << endl;
    
    /// calculate absolute points
    float x = 0, y = 0;
    float regionx = 0, regiony = 0;
    for (size_t i=0; i < lines.size(); i++) {
      regionx = ( (float)lines[i].region.column* (float)_kernel_size);
      regiony = ( (float)lines[i].region.row* (float)_kernel_size);
      x = regionx + lines[i].p1.x();
      y = regiony + lines[i].p1.y();
      
      //cout << x << " AND " << y << endl;
      
      lines[i].p1 = t_point2d{static_cast<float_t>(x),static_cast<float_t>(y)};
      x = regionx + lines[i].p2.x();
      y = regiony + lines[i].p2.y();
      
      lines[i].p2 = t_point2d{static_cast<float_t>(x),static_cast<float_t>(y)};
      
      file << "LINE " << i << endl;
      
      file << "P1 " << lines[i].p1 << endl;
      
      file << "P2 " << lines[i].p2 << endl;
      
      file << "-------- " << endl;
      
    }
    
    return false;
  }
  
  bool toImage(shared_ptr<Image> outputImage){
    
    outputImage->Format = PixelFormat::RGBA;
    outputImage->BitsPerComponent = 8;
    outputImage->BitsPerPixel = 32;
    outputImage->BytesPerPixel = 4;
    outputImage->Height = _kernel_size*_line_matrix.rows();
    outputImage->Width = _kernel_size*_line_matrix.cols();
    outputImage->BytesPerRow = outputImage->Width*outputImage->BytesPerPixel;
    outputImage->Pixels.resize(outputImage->BytesPerRow*outputImage->Height, 0xFF);
    
    t_lineVector lines;
    
    if (!toVector(lines))
      return false;
    /// calculate absolute points
    size_t x = 0, y = 0, regionx = 0, regiony = 0;
    for (size_t i=0; i < lines.size(); i++) {
      regionx = (lines[i].region.column*_kernel_size);
      regiony = (lines[i].region.row*_kernel_size);
      x = regionx + lines[i].p1.x();
      y = regiony + lines[i].p1.y();
      lines[i].p1 = t_point2d{static_cast<float_t>(x),static_cast<float_t>(y)};
      x = regionx + lines[i].p2.x();
      y = regiony + lines[i].p2.y();
      lines[i].p2 = t_point2d{static_cast<float_t>(x),static_cast<float_t>(y)};
    }
    
#if defined(__APPLE__) || defined(__MACH__)
    /// draw lines to image
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGBitmapInfo info = kCGImageAlphaNone;
    switch(outputImage->Format){
      case PixelFormat::ARGB:
      {
        info = kCGImageAlphaNoneSkipFirst;
        break;
      }
      case PixelFormat::RGBA:
      {
        info = kCGImageAlphaNoneSkipLast;
        break;
      }
      default:
      {
        CGColorSpaceRelease(colorSpace);
        return false;
      }
    }
    CGContextRef context = CGBitmapContextCreate(outputImage->Pixels.data(),
                                                 outputImage->Width,
                                                 outputImage->Height,
                                                 outputImage->BitsPerComponent,
                                                 outputImage->BytesPerRow,
                                                 colorSpace, info);
    //CGContextBeginPath(context);
    CGFloat components[]{0,0,0,255};
    
    CGColorRef strokeColor =  CGColorCreate(colorSpace, components);//[_strokeColor CGColor];
    
    CGColorSpaceRelease(colorSpace);
    
    //CGColorRef fillColor = [UIColor redColor].CGColor;// [_bgColor CGColor];
    //CGContextSetFillColorWithColor(context, fillColor);
    CGContextBeginPath(context);
    CGContextSaveGState(context);
    CGContextTranslateCTM(context, 0, outputImage->Height);
    CGContextScaleCTM(context, 1.0, -1.0);
    cout << " THERE ARE " << lines.size();
    for (size_t i=0; i < lines.size(); i++) {
      CGContextMoveToPoint(context, lines[i].p1.x() , lines[i].p1.y());
      CGContextAddLineToPoint(context, lines[i].p2.x(), lines[i].p2.y());
      //CGContextDrawPath(context, kCGPathFillStroke);
    }
    CGContextSetLineWidth(context, 1.0f);
    CGContextSetInterpolationQuality(context, kCGInterpolationNone);
    CGContextSetFlatness(context, 1.0f);
    CGContextSetAllowsAntialiasing(context, false);
    CGContextSetShouldSmoothFonts(context,false);
    
    //CGContextSetLineCap(context, kCGLineCapSquare);
    //CGContextSetLineJoin(context, kCGLineJoinMiter);
    CGContextSetFillColorWithColor(context, strokeColor);
    CGContextSetStrokeColorWithColor(context, strokeColor);
    CGContextStrokePath(context);
    CGContextRestoreGState(context);
    //CGContextFlush(context);
#endif
    
    return true;
  }
  
  bool toVector(t_lineVector& outputLines){
    /// check if ready
    for (size_t i=0; i < _line_matrix.rows(); i++) {
      for (size_t j=0; j < _line_matrix.cols(); j++) {
        outputLines.insert(outputLines.end(), _line_matrix(i,j).begin(), _line_matrix(i,j).end());
      }
    }
    return true;
  }
  
  template<typename E>
  void random_line(t_line2d& line,
                   E& engine){
    /// random points
    size_t min = 0, max = _kernel_size-1;
    uniform_int_distribution<uint32_t> gen(min,max);
    float_t length = 0;
    size_t x =0, y =0;
    while (true) {
      x = gen(engine); y = gen(engine);
      cout << " X1 " << x << " Y1 " << y << endl;
      t_point2d p1{static_cast<float_t>(x),static_cast<float_t>(y)};
      
      x = gen(engine); y = gen(engine);
      cout << " X2 " << x << " Y2 " << y << endl;
      t_point2d p2{static_cast<float_t>(x),static_cast<float_t>(y)};
      length = computeLength(p1.x(), p1.y(), p2.x(), p2.y());
      cout << " LENGTH : " << length;
      if (length < _minimum_length)
        continue;
      line.p1 = p1;
      line.p2 = p2;
      break;
    }
  }
  
  explicit random_lines(const size_t    kernelSize,
                        const float_t   minimumLength,
                        const size_t    minimumLines,
                        const size_t    maximumLines):
  _kernel_size(kernelSize),
  _random_pp(bind(&random_lines::processRow, ref(*this)),
             bind(&random_lines::done,ref(*this))){
    _processed_row = 0;
    _minimum_length = minimumLength;
    _minimum_lines = minimumLines;
    _maximum_lines  = maximumLines;
  }
  
private:
  
  typedef struct RowState{
    t_lineVector  _lines;
    size_t        _row;
    size_t        _col;
    random_device _rd;
  }RowState;
  
  bool done(){
    lock_guard<mutex> l(_row_mutex);
    return _processed_row >= _line_matrix.rows();
  }
  
  void processRow(){
    RowState state;
    _row_mutex.lock();
    state._row = _processed_row;
    _processed_row++;
    _row_mutex.unlock();
    
    if (state._row >= _line_matrix.rows()) return;
    
    size_t    lines = 0;
    t_line2d  line;
    mt19937   engine(state._rd());
    uniform_int_distribution<> gen(_minimum_lines,_maximum_lines);
    for (state._col = 0; state._col<_line_matrix.cols(); state._col++) {
      /// generate number of lines
      lines = gen(engine);
      for (size_t n = 0; n < lines; n++) {
        random_line(line,engine);
        line.region.row = state._row;
        line.region.column = state._col;
        state._lines.push_back(line);
      }
      _line_matrix_mutex.lock();
      _line_matrix(state._row,state._col) = move(state._lines);
      _line_matrix_mutex.unlock();
    }
  }
  
  void processRows(){
    while (!done()) {
      processRow();
    }
  }
  
  parallel_process<random_lines>                                _random_pp;
  mutex                                                         _row_mutex;
  size_t                                                        _processed_row;
  size_t                                                        _minimum_lines;
  size_t                                                        _maximum_lines;
  float_t                                                       _minimum_length;
  mutex                                                         _line_matrix_mutex;
  Eigen::Matrix<t_lineVector, Eigen::Dynamic, Eigen::Dynamic>   _line_matrix;
  const size_t                                                  _kernel_size;
};

template<class P>
class hough_line_dataset : public matrix_parallel_process<hough_line_dataset<P>>{
  struct RowState{
    char*     pixelPtr;
    char*     bufferPtr;
    size_t    componentsSum;
    t_pointVector points;
    
  };
  typedef struct RowState RowState;
  typedef matrix_parallel_process<hough_line_dataset<P>> BaseType;
  
  void Kernel(typename BaseType::RowState* rs){
    //cout << rs->row << " " << rs->column << endl;
    //cout.flush();
    RowState* state = dynamic_cast<RowState*>(rs);
    
    state->bufferPtr = (char*)image->Pixels.data()+(rs->row*(image->BytesPerRow*kernelSize) + (rs->column*image->BytesPerPixel* kernelSize));
    //cout << kernelSize << " IS " << endl;
    for (size_t y=0; y<kernelSize; y++) {
      state->pixelPtr = state->bufferPtr;
      for (size_t x=0; x<kernelSize; x++) {
        P pixel(state->pixelPtr);
        state->componentsSum = pixel.red;
        state->componentsSum += pixel.green;
        state->componentsSum += pixel.blue;
        
        if (state->componentsSum <= pixelThreshold){
          state->points.push_back(t_point2d{static_cast<float_t>(x),static_cast<float_t>(y)});
          //cout << "X ";
        }//else
        //cout << ". ";
        
        //cout << state->componentsSum << " ";
        state->pixelPtr += image->BytesPerPixel;
      }
      state->bufferPtr += image->BytesPerRow;
      //cout << endl;
    }
    
    if (state->points.size() > 1 ){
      cv::Mat cvimage(kernelSize,kernelSize,CV_8UC1,Scalar(0,0,0));
      
      for (size_t i=0;
           i<state->points.size();
           i++) {
        cvimage.at<uchar>(state->points[i].y(),state->points[i].x()) = 255;
      }
      vector<Vec2f> lines;
      HoughLines( cvimage, lines, distanceResolution, thetaResolution, accumulatorThreshold );
      t_lineVector hlines;
      cv::Mat dst = cvimage;
      size_t half_kernel = kernelSize*0.5;
      for( size_t i = 0; i < lines.size(); i++ )
      {
        float rho = lines[i][0];
        float theta = lines[i][1];
        double a = cos(theta), b = sin(theta);
        
        //cout << rho * a << " " << rho * b << endl;
        //cout << a << " " << b << endl;
        double x0 = a*rho, y0 = b*rho;
        //cout << x0 << " "  << y0 << endl;
        /*
         cv::Point pt1(cvRound(x0 + kernelSize*(-b)),
         cvRound(y0 + kernelSize*(a)));
         cv::Point pt2(cvRound(x0 - kernelSize*(-b)),
         cvRound(y0 - kernelSize*(a)));
         cout << pt1 << " " << pt2 << endl;
         */
        t_point2d p1{
          (float_t)cvRound(x0 + half_kernel*(-b)),
          (float_t)cvRound(y0 + half_kernel*(a))
        };
        t_point2d p2{
          (float_t)cvRound(x0 - half_kernel*(-b)),
          (float_t)cvRound(y0 - half_kernel*(a))
        };
        //
        //        cv::Point pt1(cvRound(x0 + 15*(-b)),
        //                      cvRound(y0 + 15*(a)));
        //        cv::Point pt2(cvRound(x0 - 15*(-b)),
        //                      cvRound(y0 - 15*(a)));
        //        cout << pt1 << " " << pt2 << endl;
        //        //line( dst, pt1, pt2, Scalar(42,54,165), 1, 4 );
        
        //cout << p1 << " " << p2 << endl;
        t_line2d hl{p1,p2}; hl.region.row = rs->row; hl.region.column = rs->column;
        hlines.push_back(hl);
        //line( dst, pt1, pt2, Scalar(42,54,165), 1, 4 );
        
      }
      hlinesmutex.lock();
      hlinesDataSet(rs->row,rs->column) = move(hlines);
      hlinesmutex.unlock();
    }
    state->points.clear();
  }
  shared_ptr<Image>     image;
  const size_t          kernelSize;
  const size_t          pixelThreshold;
  const float_t         accumulatorThreshold;
  mutex                   hlinesmutex;
  float_t         distanceResolution;
  float_t         thetaResolution;
  Eigen::Matrix<t_lineVector, Eigen::Dynamic, Eigen::Dynamic>           hlinesDataSet;
public:
  
  void setDistanceResolution(float_t rho){
    distanceResolution = rho;
  }
  
  void setThetaResolution(float_t theta){
    thetaResolution = theta;
  }
  
  friend class matrix_parallel_process<hough_line_dataset<P>>;
  explicit hough_line_dataset(shared_ptr<Image> image,
                             const size_t kernelSize,
                             const size_t pixelThreshold,
                             const float_t accumulatorThreshold,
                             const float_t RHO,
                             const float_t THETA):
  BaseType(image->Height/kernelSize,
           image->Width/kernelSize),
  image(image),
  kernelSize(kernelSize),
  pixelThreshold(pixelThreshold),
  accumulatorThreshold(accumulatorThreshold),
  distanceResolution(RHO),
  thetaResolution(THETA){
    hlinesDataSet.resize(image->Height/kernelSize,image->Width/kernelSize);
  }
  
  bool toVector(t_lineVector& outputLines){
    /// check if ready
    for (size_t i=0; i < hlinesDataSet.rows(); i++) {
      for (size_t j=0; j < hlinesDataSet.cols(); j++) {
        outputLines.insert(outputLines.end(), hlinesDataSet(i,j).begin(), hlinesDataSet(i,j).end());
      }
    }
    return true;
  }
  
  shared_ptr<Image> getImage(){
    return image;
  }
  
  bool toFile(string filepath){
    
    ofstream file(filepath);
    
    if (!file.is_open()){
      return false;
    }
    
    t_lineVector lines;
    
    if (!toVector(lines))
      return false;
    
    file << "PRINTING " << lines.size() << " DETECTED " << endl;
    
    float x = 0, y = 0, regionx = 0, regiony = 0;
    for (size_t i=0; i < lines.size(); i++) {
      regionx = ( (float)lines[i].region.column*kernelSize);
      regiony = ( (float)lines[i].region.row*kernelSize);
      x = regionx + lines[i].p1.x();
      y = regiony + lines[i].p1.y();
      //cout << x << " AND " << y << endl;
      lines[i].p1 = t_point2d{static_cast<float_t>(x),static_cast<float_t>(y)};
      x = regionx + lines[i].p2.x();
      y = regiony + lines[i].p2.y();
      //cout << x << " AND " << y << endl;
      lines[i].p2 = t_point2d{static_cast<float_t>(x),static_cast<float_t>(y)};
      file << "LINE " << i << endl;
      file << "P1 " << lines[i].p1 << endl << "P2 " << lines[i].p2 << endl;
      file << "--------" << endl;
    }
    
    file.close();
    
    return true;
  }
  
  bool toImage(shared_ptr<Image> outputImage = nullptr){
    
    if (outputImage != nullptr){
      outputImage->Format = PixelFormat::RGBA;
      outputImage->BitsPerComponent = 8;
      outputImage->BitsPerPixel = 32;
      outputImage->BytesPerPixel = 4;
      outputImage->Height = kernelSize*hlinesDataSet.rows();
      outputImage->Width = kernelSize*hlinesDataSet.cols();
      outputImage->BytesPerRow = outputImage->Width*outputImage->BytesPerPixel;
      outputImage->Pixels.resize(outputImage->BytesPerRow*outputImage->Height, 0xFF);
    }else{
      outputImage = image;
    }
    
    t_lineVector lines;
    
    if (!toVector(lines))
      return false;
    
    cout << " THERE ARE " << lines.size() << endl;
    
    /// calculate absolute points
    size_t x = 0, y = 0, regionx = 0, regiony = 0;
    for (size_t i=0; i < lines.size(); i++) {
      regionx = (lines[i].region.column*kernelSize);
      regiony = (lines[i].region.row*kernelSize);
      x = regionx + lines[i].p1.x();
      y = regiony + lines[i].p1.y();
      lines[i].p1 = t_point2d{static_cast<float_t>(x),static_cast<float_t>(y)};
      x = regionx + lines[i].p2.x();
      y = regiony + lines[i].p2.y();
      lines[i].p2 = t_point2d{static_cast<float_t>(x),static_cast<float_t>(y)};
    }
    
#if defined(__APPLE__) || defined(__MACH__)
    /// draw lines to image
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGBitmapInfo info = kCGImageAlphaNone;
    switch(outputImage->Format){
      case PixelFormat::ARGB:
      {
        info = kCGImageAlphaNoneSkipFirst;
        break;
      }
      case PixelFormat::RGBA:
      {
        info = kCGImageAlphaNoneSkipLast;
        break;
      }
      default:
      {
        CGColorSpaceRelease(colorSpace);
        return false;
      }
    }
    CGContextRef context = CGBitmapContextCreate(outputImage->Pixels.data(),
                                                 outputImage->Width,
                                                 outputImage->Height,
                                                 outputImage->BitsPerComponent,
                                                 outputImage->BytesPerRow,
                                                 colorSpace, info);
    CGFloat components[]{1,0,0,1};
    CGColorRef strokeColor =  CGColorCreate(colorSpace, components);
    CGColorSpaceRelease(colorSpace);
    CGContextTranslateCTM(context, 0, outputImage->Height);
    CGContextScaleCTM(context, 1.0, -1.0);
    CGContextSetLineWidth(context, 0.5f);
    CGContextSetInterpolationQuality(context, kCGInterpolationNone);
    CGContextSetShouldAntialias(context, false);
    CGContextSetAllowsAntialiasing(context, false);
    CGContextSetShouldSmoothFonts(context,false);
    CGContextSetFillColorWithColor(context, strokeColor);
    CGContextSetStrokeColorWithColor(context, strokeColor);
    
    for (size_t i=0; i < lines.size(); i++) {
      CGContextMoveToPoint(context, lines[i].p1.x() , lines[i].p1.y());
      CGContextAddLineToPoint(context, lines[i].p2.x(), lines[i].p2.y());
    }
    
    CGContextSetFillColorWithColor(context, strokeColor);
    CGContextSetStrokeColorWithColor(context, strokeColor);
    CGContextStrokePath(context);
    CGContextRelease(context);
#endif
    
    return true;
  }
};



#endif
