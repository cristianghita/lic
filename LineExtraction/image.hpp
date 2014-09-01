//
//  image.hpp
//  LineExtraction
//
//  Created by cristi on 8/5/14.
//  Copyright (c) 2014 cristi. All rights reserved.
//

#ifndef LineExtraction_image_hpp
#define LineExtraction_image_hpp

#include "Common.hpp"
#include "util.hpp"

typedef enum class PixelFormat : int{
  RGBA,
  ARGB
}PixelFormat;

template< PixelFormat P >
inline unsigned char red(int32_t*){ return 0;}
template< PixelFormat P >
inline unsigned char green(int32_t*){ return 0;}
template< PixelFormat P >
inline unsigned char blue(int32_t*){ return 0;}
template< PixelFormat P >
inline unsigned char alpha(int32_t*){ return 0;}

template<>
inline unsigned char red<PixelFormat::RGBA>(int32_t* p){
  return ((*p))&0xff;
}
template<>
inline unsigned char green<PixelFormat::RGBA>(int32_t* p){
  return ((*p)>>8)&0xff;
}
template<>
inline unsigned char blue<PixelFormat::RGBA>(int32_t* p){
  return ((*p)>>16)&0xff;
}
template<>
inline unsigned char alpha<PixelFormat::RGBA>(int32_t* p){
  return ((*p)>>24)&0xff;
}
template<>
inline unsigned char alpha<PixelFormat::ARGB>(int32_t* p){
  return ((*p)>>24)&0xff;
}
template<>
inline unsigned char red<PixelFormat::ARGB>(int32_t* p){
  return ((*p)>>16)&0xff;
}
template<>
inline unsigned char green<PixelFormat::ARGB>(int32_t* p){
  return ((*p)>>8)&0xff;
}
template<>
inline unsigned char blue<PixelFormat::ARGB>(int32_t* p){
  return ((*p))&0xff;
}

typedef struct RGBAS{
  static const size_t BytesPerPixel = 4;
  unsigned char   red;
  unsigned char   green;
  unsigned char   blue;
  unsigned char   alpha;
  RGBAS(void* p){
    red = ::red<PixelFormat::RGBA>((int32_t*)p );
    green = ::green<PixelFormat::RGBA>((int32_t*)p);
    blue = ::blue<PixelFormat::RGBA>((int32_t*)p);
    alpha = ::alpha<PixelFormat::RGBA>((int32_t*)p);
  }
  RGBAS():red(0),green(0),blue(0),alpha(0){}
}RGBA;

typedef struct ARGBS{
  static const size_t BytesPerPixel = 4;
  unsigned char red;
  unsigned char green;
  unsigned char blue;
  unsigned char alpha;
  ARGBS(void* p){
    red = ::red<PixelFormat::ARGB>((int32_t*)p);
    green = ::green<PixelFormat::ARGB>((int32_t*)p);
    blue = ::blue<PixelFormat::ARGB>((int32_t*)p);
    alpha = ::alpha<PixelFormat::ARGB>((int32_t*)p);
  }
}ARGB;

struct Image {

  typedef enum : int32_t {
    JPEG,
    PNG,
    Unknown
  } ImageType;
  
  Image&& operator<< ( cv::Mat& cvImage ){
    
    cout << "Conversion operator called " << endl;
    
    /// set default properties for image
    this->BitsPerPixel = 32;
    this->BitsPerComponent = 8;

    this->BytesPerPixel = 32/8;
    this->Width = cvImage.cols;
    this->Height = cvImage.rows;
    
    switch(cvImage.type()){
      case CV_8UC1:
      {
        cout << "OpenCV image is 8UC1" << endl;
        this->BytesPerRow = 4*cvImage.cols;
        this->Pixels.resize(cvImage.rows*cvImage.cols * this->BytesPerPixel);
        unsigned char* pixelptr = NULL;
        
        for(size_t r=0; r < cvImage.rows; r++ ){
          for(size_t c=0; c < cvImage.cols; c++){
            pixelptr = &this->Pixels[r*this->BytesPerRow + (c*this->BytesPerPixel)];
            pixelptr[0] = cvImage.at<uchar>(r,c);
            pixelptr[1] = cvImage.at<uchar>(r,c);
            pixelptr[2] = cvImage.at<uchar>(r,c);
            pixelptr[3] = 0xFF;
          }
        }
        
        break;
      }
      default:
        break;
    }
    
    return move(*this);
  }
  
  bool applySobel( string path , const size_t ks = 3){
    cv::Mat readImage = cv::imread(path.c_str(),0);
    return applySobel(readImage,ks);
  }
  
  bool applyCanny( string path , bool blur, const size_t ks = 3){
    cv::Mat readImage = cv::imread(path.c_str(),0);
    return applyCanny(readImage,blur,ks);
  }
  
  bool applySobel( cv::Mat& cvImage, const size_t ks = 3){
    
    if (cvImage.empty()) return false;
    
    cv::Sobel(cvImage, cvImage, -1, 1, 1, ks);
    
    *this << cvImage;
    
    return true;
  }
  
  bool applyCanny( cv::Mat& cvImage, bool blur, const size_t ks = 3){
    
    if (cvImage.empty()) return false;
    
    cv::Mat readImage = cvImage;
    
    if( blur )
      cv::blur(readImage,cvImage,cv::Size(ks,ks));
    
    cv::Canny(cvImage,cvImage,100,300,ks);
    
    *this << cvImage;
    
    return true;
  }
  
  static shared_ptr<Image> FromFile(string path){
    shared_ptr<Image>   image(new Image);
#if defined(__MACH__) || defined(__APPLE__)
    
    ImageType type = Unknown;
    string jpeg_ext(".jpeg"), png_ext(".png"), jpg_ext(".jpg");
    if (path.find(jpeg_ext) != string::npos || path.find(jpg_ext) != string::npos){
      type = JPEG;
    }
    
    if (path.find(png_ext) != string::npos){
      type = PNG;
    }
    CGDataProviderRef dataprovider = CGDataProviderCreateWithFilename(path.c_str());
    CGImageRef          bitmap;
    if (dataprovider==NULL){
      return nullptr;
    }
    
    switch(type){
      case PNG:
      {
        bitmap = CGImageCreateWithPNGDataProvider(dataprovider, NULL, false, kCGRenderingIntentDefault);
        break;
      }
      case JPEG:
      {
        bitmap = CGImageCreateWithJPEGDataProvider(dataprovider, NULL, false, kCGRenderingIntentDefault);
        break;
      }
      default:
      {
        return nullptr;
        break;
      }
    };

    image->BitsPerPixel = CGImageGetBitsPerPixel(bitmap);
    image->BitsPerComponent = CGImageGetBitsPerComponent(bitmap);
    image->BytesPerRow = CGImageGetBytesPerRow(bitmap);
    image->BytesPerPixel = image->BitsPerPixel/8;
    image->Width = CGImageGetWidth(bitmap);
    image->Height = CGImageGetHeight(bitmap);
    
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(NULL, image->Width, image->Height, image->BitsPerComponent, image->BytesPerRow, colorSpace, CGImageGetBitmapInfo(bitmap));
    CGColorSpaceRelease(colorSpace);
    if (!ctx) return nullptr;
    CGRect rect = {{0.0f,0.0f},{(float)image->Width,(float)image->Height}};
    CGContextDrawImage(ctx, rect, bitmap);
    const size_t len = image->BytesPerRow*image->Height;
    image->Pixels.resize(len);
    const char* ptr = (const char*) CGBitmapContextGetData(ctx);
    memcpy(image->Pixels.data(), ptr, len);
    CGContextRelease(ctx);
    
    switch(CGImageGetAlphaInfo(bitmap)){
      case kCGImageAlphaFirst:{
        image->Format = PixelFormat::ARGB;
        break;
      }
      case kCGImageAlphaNoneSkipFirst:{
        image->Format = PixelFormat::ARGB;
        break;
      }
      case kCGImageAlphaNoneSkipLast:{
        image->Format = PixelFormat::RGBA;
        break;
      }
      default:{
        break;
      }
    };
    
    CGImageRelease(bitmap);
#endif
    return image;
  }
  
  bool ToJPEGFile(string path,PixelFormat format){
#if defined(__MACH__) || defined(__APPLE__)
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    
    CGBitmapInfo info = kCGImageAlphaNone;
    switch(format){
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
    
    CGContextRef ctx = CGBitmapContextCreate(Pixels.data(), Width, Height, BitsPerComponent, BytesPerRow, colorSpace, info);
    CGColorSpaceRelease(colorSpace);
    if (ctx == NULL ) return false;
    CGImageRef image = CGBitmapContextCreateImage(ctx);
    if (!image){
      CGContextRelease(ctx);
      return false;
    }
    CGContextRelease(ctx);
    CGImageDestinationRef cgImageDest = NULL;
    CFURLRef cfURL = NULL;
    CFStringRef cfStrPath = CFStringCreateWithCString( kCFAllocatorDefault,
                                                      path.c_str(),
                                                      kCFStringEncodingASCII);
    if (!cfStrPath){
      CGImageRelease(image);
      return false;
    }
    
    cfURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                          cfStrPath,
                                          kCFURLPOSIXPathStyle,
                                          false);
    if (!cfURL){
      CFRelease(cfStrPath);
      CGImageRelease(image);
      return false;
    }
    
    cgImageDest = CGImageDestinationCreateWithURL(cfURL,
                                                  kUTTypeJPEG,
                                                  1,
                                                  NULL);
    if (!cgImageDest){
      CFRelease(cfStrPath);
      CFRelease(cfURL);
      CGImageRelease(image);
      return false;
    }
    
    CGImageDestinationAddImage(cgImageDest, image, NULL);
    
    if (!CGImageDestinationFinalize(cgImageDest)){
      
      CFRelease(cfStrPath);
      CFRelease(cfURL);
      CFRelease(cgImageDest);
      CGImageRelease(image);
      return false;
    }
    
    CFRelease(cfStrPath);
    CFRelease(cfURL);
    CFRelease(cgImageDest);
    CGImageRelease(image);
    return true;
#endif
  }
  
  size_t  BitsPerPixel;
  size_t  BytesPerPixel;
  size_t  BytesPerRow;
  size_t  BitsPerComponent;
  size_t  Width;
  size_t  Height;
  PixelFormat Format;
  vector<unsigned char>   Pixels;
};


#endif
