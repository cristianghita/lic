//
//  Common.hpp
//  LineExtraction
//
//  Created by cristi on 8/5/14.
//  Copyright (c) 2014 cristi. All rights reserved.
//

#ifndef LineExtraction_Common_hpp
#define LineExtraction_Common_hpp

/// INCLUDES
#include "Eigen/Sparse"
#include "Eigen/LU"
#include "Log.hpp"
#include <thread>
#include <algorithm>
#include <mutex>
#include <vector>
#include <fstream>
#include <array>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <random>

/// TEMP
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#if defined(__MACH__) || defined(__APPLE__)
#include <unistd.h>
#include <sys/sysctl.h>
#include <ApplicationServices/ApplicationServices.h>

#elif defined(__ANDROID__)

#endif

using namespace std;

/// STATIC/DEFINES
#define KERNEL_SIZE 25
#define SLOPE_THRESHOLD 0.1f
#define INTERCEPT_THRESHOLD 400
#undef INFINITY
#define INFINITY 1000
#define KERNEL_LINE_OCCURENCE 5
#define KERNEL_LINE_ERROR_PERCENT 10
#define THREADS 1
#define DEG2RAD 0.017453293f


#define for_permutation(i,j,s) \
for(i=0; i<=s; i++) \
for(j=i;j<=s-1;j++)


template<typename T>
ostream& operator<<(ostream& os, vector<T>& c){
  for( size_t i=0; i<c.size();i++){
    os << c[i] << endl;
    os.flush();
  }
  return os;
}


typedef vector<size_t> t_indexVector;

t_indexVector operator-( t_indexVector& first , t_indexVector& second ){
  
  t_indexVector output;
  
  for (t_indexVector::iterator i = first.begin(); i != first.end(); i++ ) {
    
    if (find(second.begin(), second.end(), *i) == second.end())
      output.push_back(*i);
  }
  
  return move(output);
}


#endif
