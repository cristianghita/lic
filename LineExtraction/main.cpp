//
//  main.cpp
//  LineExtraction
//
//  Created by cristi on 8/5/14.
//  Copyright (c) 2014 cristi. All rights reserved.
//

#include <iostream>
#include "clustering/clustering.hpp"
#include "mt/util.hpp"
#include "image.hpp"
#include "lineextract.hpp"

inline float computeSlope ( float_t x1, float_t y1, float_t x2, float_t y2 ){
  float slopevalue = 1.0f;
  if ( x1 == x2 ){
    return (float)1001.0f;//41.0f;//(float)((y1+y2)/2);//(float)KERNEL_SIZE;//41.0f;
  }
  
  if (x1 == 2.5 &&
      y1 == 1.5 &&
      x2 == 3 &&
      y2 == 2.5){
    cout << "AICI" << endl;
  }
  
  float_t nominator = y2 - y1;
  float_t denominator = x2 - x1;
  slopevalue = nominator / denominator;
  if (slopevalue == -0.0f)
    slopevalue = 0.0f;
  return slopevalue;
}

using namespace cv;

typedef struct ColinearDistance{
  
  typedef float_t distance_type;
  
  float_t   td;
  float_t   ts;
  float_t   tc;
  
  ColinearDistance(float_t nd,
                   float_t ad,
                   float_t maa):
  td(nd),
  ts(ad),
  tc(maa){}
  
  ColinearDistance() = default;
  
  distance_type operator()(t_line2d& l1,
                           t_line2d& l2){
    
    float_t avgLength = (l1.length()+l2.length())*0.5f;
    l1.normalize();
    l2.normalize();
    
    float_t length_l1l2p1 = l1.p1.lengthBetween(l2.p1);
    float_t length_l1l2p2 = l1.p2.lengthBetween(l2.p2);
    float_t d = length_l1l2p1;
    float_t s = 0.0f;
    float_t c = 0.0f;
    
    t_line2d cline{ l1.centerPoint(), l2.centerPoint()};
    cline.normalize();
    
    /// calculate s
    if (length_l1l2p1 >= length_l1l2p2){
      /// from p2 to p1
      t_point2d v1{l1.p1.x()-l1.p2.x(),l1.p1.y() - l1.p2.y()},
                v2{l2.p1.x()-l2.p2.x(),l2.p1.y() - l2.p2.y()};
      
      s = fabs( v1.x()* v2.x() + v1.y()*v2.y() );
      s = s/(sqrtf(powf(v1.x(), 2)+powf(v1.y(), 2))*sqrtf(powf( v2.x(), 2)+powf(v2.y(), 2)));
      d = length_l1l2p2;
      s = acos(s)*DEG2RAD;
      /// create the vector c1 to c2
      t_point2d v{cline.p2.x() - cline.p1.x(), cline.p2.y() - cline.p1.y()};
      
      float_t c1 = fabs(v1.x()*v.x() + v1.y()*v.y());
      c1 = c1/(sqrtf(powf(v1.x(), 2)+powf(v1.y(), 2)) * sqrtf(powf(v.x(), 2)+powf(v.y(), 2)));
      c1 = acos(c1)*DEG2RAD;
      v.x() = cline.p1.x() - cline.p2.x();
      v.y() = cline.p1.y() - cline.p2.y();
      
      float_t c2 = fabs(v1.x()*v.x() + v1.y()*v.y());
      c2 = c2/(sqrtf(powf(v1.x(), 2)+powf(v1.y(), 2)) * sqrtf(powf(v.x(), 2)+powf(v.y(), 2)));
      c2 = acos(c2)*DEG2RAD;
      c = (c1+c2)*0.5f;
      
    }else{
      t_point2d v1{l1.p2.x()-l1.p1.x(), l1.p2.y() - l1.p1.y()},
                v2{l2.p2.x()-l2.p1.x(), l2.p2.y() - l2.p2.y()};
      s = fabs( v1.x()* v2.x() + v1.y()*v2.y() );
      s = s/(sqrtf(powf(v1.x(), 2)+powf(v1.y(), 2))*sqrtf(powf( v2.x(), 2)+powf(v2.y(), 2)));
      s = acos(s)*DEG2RAD;
      /// create the vector c1 to c2
      t_point2d v{cline.p2.x() - cline.p1.x(), cline.p2.y() - cline.p1.y()};
      
      float_t c1 = fabs(v1.x()*v.x() + v1.y()*v.y());
      c1 = c1/(sqrtf(powf(v1.x(), 2)+powf(v1.y(), 2)) * sqrtf(powf(v.x(), 2)+powf(v.y(), 2)));
      c1 = acos(c1)*DEG2RAD;
      v.x() = cline.p1.x() - cline.p2.x();
      v.y() = cline.p1.y() - cline.p2.y();
      
      float_t c2 = fabs(v1.x()*v.x() + v1.y()*v.y());
      c2 = c2/(sqrtf(powf(v1.x(), 2)+powf(v1.y(), 2)) * sqrtf(powf(v.x(), 2)+powf(v.y(), 2)));
      c2 = acos(c2)*DEG2RAD;
      c = (c1+c2)*0.5f;
    }
    
    float_t dist = avgLength*( (td-(d/avgLength)) * (ts-s) * (tc-c) );
    return dist;
  }
  
}ColinearDistance;

template<typename O>
class SingleLinkage{
  typename O::distance_type minimum;
public:
  explicit SingleLinkage(typename O::distance_type value) : minimum(value){}
  bool operator()(typename O::distance_type d){
    if ( d <= minimum ){
      minimum = d;
      return true;
    }
    return false;
  }
  typename O::distance_type Value(){ return minimum; }
  void Value(typename O::distance_type d) { minimum = d; }
};

typedef struct EuclideanDistance{
  
  typedef float_t distance_type;
  
  distance_type operator()(const t_point2d& p1,
                           const t_point2d& p2){
    return p1.lengthBetween(p2);
  }
  
  distance_type infinity(){
    return 42.0f;
  }
  
  EuclideanDistance& operator=(const EuclideanDistance&) = default;
  EuclideanDistance() = default;
  
}EuclideanDistance;


void testRandomLines(){
  random_lines rl(35, 11, 1, 2);
  
  rl.generate(1, 15 , 15 );
  
  shared_ptr<Image> imageptr(new Image);
  
  rl.toFile("/Volumes/External/random_lines.txt");
  
  rl.toImage(imageptr);
  
  imageptr->ToJPEGFile("/Volumes/External/toimg.jpg", PixelFormat::RGBA);
}

void testEuclideanDistance(){
  
  vector<t_point2d> points;
  points.push_back(t_point2d{1,1});
  points.push_back(t_point2d{1,2});
  points.push_back(t_point2d{2,2});
  points.push_back(t_point2d{5,6});
  points.push_back(t_point2d{6,7});
  
  AgglomerativeHierarchical<vector<t_point2d>, EuclideanDistance, SingleLinkage<EuclideanDistance>> aggh(points.begin(),points.end());
  
  t_indexVector indexes;
  float_t distance = 0.0f;
  while (aggh.nextClusters()) {
    
    aggh.currentClusterIndexes(move(indexes));
    aggh.currentClusterDistance(distance);
    
    cout << distance << endl;
  
    for (t_indexVector::iterator i = indexes.begin(); i!=indexes.end(); i++) {
      cout << points[*i] << ";";
    }
    
    cout << endl;
  }
}

void testFilters(){
  Image image;
  
  if (!image.applySobel("/Volumes/External/Box1.jpg",5)){
    cout << "Failed to apply filter";
  }
  
  image.ToJPEGFile("/Volumes/External/Box1Sobel2.jpg", PixelFormat::RGBA);
  
  if (!image.applyCanny("/Volumes/External/Box1.jpg",true,5)){
    cout << "Failed to apply Canny" << endl;
  }
  
  image.ToJPEGFile("/Volumes/External/Box1Canny1.jpg", PixelFormat::RGBA);
}

void testHoughLines(){
  hough_line_dataset<RGBA> hlds(Image::FromFile("/Volumes/External/toimg.jpg"),
                                KERNEL_SIZE,
                                255,
                                KERNEL_SIZE*0.5,
                                1,
                                CV_PI/18);
  START_CLOCK
  hlds.run(1);
  
  hlds.wait(chrono::seconds(1000));
  STOP_CLOCK(milliseconds,"HL")
  //shared_ptr<Image> output_image(new Image);
  
  if (!hlds.toImage(nullptr)){
    cout << "Failed to output to image" << endl;
    return;
  }
  /*
   if (!output_image->ToJPEGFile("/Volumes/External/output_houghlines.jpg", PixelFormat::RGBA)){
   cout << "FAILED TO WRITE" << endl;
   }
   */
  
  hlds.getImage()->ToJPEGFile("/Volumes/External/output_hlines.jpg", PixelFormat::RGBA);
  
  if (!hlds.toFile("/Volumes/External/line.txt") ){
    cout << " FAiled to write lines to file " << endl;
  }
}

int main(int argc, const char * argv[]) {
  
  //testEuclideanDistance();
  
  //testRandomLines();

  testHoughLines();
  
  testFilters();
  
  return 0;
}
