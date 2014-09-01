//
//  Log.hpp
//  LineExtraction
//
//  Created by cristi on 8/5/14.
//  Copyright (c) 2014 cristi. All rights reserved.
//

#ifndef LineExtraction_Log_hpp
#define LineExtraction_Log_hpp

#include <iostream>

#define _DEBUG

#ifdef _DEBUG
#define LOG(message) std::cout << message << std::endl;
#define LOGERROR(message) std::cerr << message << std::endl;
#else
#define LOG(message) {};
#define LOGERROR(message) {};
#endif


#define START_CLOCK \
{\
auto start_clock = std::chrono::high_resolution_clock::now();

#define STOP_CLOCK(DURATION,TITLE) \
auto end_clock = std::chrono::high_resolution_clock::now();\
std::cout << std::endl << " CLOCK COUNT FOR " << TITLE << " IS " << std::chrono::duration_cast<std::chrono::DURATION>(end_clock - start_clock).count() << std::endl;}


#endif
