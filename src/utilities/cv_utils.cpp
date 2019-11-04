#include <stdio.h>
#include <string>

#include "base/definitions.h"
#include "utilities/utils.h"
#include "utilities/cv_utils.h"

using namespace std;

std::string cvtype2str(int type){
     std::string r;
     uchar depth = type & CV_MAT_DEPTH_MASK;
     uchar chans = 1 + (type >> CV_CN_SHIFT);

     switch(depth){
          case CV_8U:  r = "8U"; break;
          case CV_8S:  r = "8S"; break;
          case CV_16U: r = "16U"; break;
          case CV_16S: r = "16S"; break;
          case CV_32S: r = "32S"; break;
          case CV_32F: r = "32F"; break;
          case CV_64F: r = "64F"; break;
          default:     r = "User"; break;
     }
     r += "C";
     r += (chans+'0');
     return r;
}
std::string cvtype2str(cv::Mat mat){
     int type = mat.type();
     std::string r;
     uchar depth = type & CV_MAT_DEPTH_MASK;
     uchar chans = 1 + (type >> CV_CN_SHIFT);

     switch(depth){
          case CV_8U:  r = "8U"; break;
          case CV_8S:  r = "8S"; break;
          case CV_16U: r = "16U"; break;
          case CV_16S: r = "16S"; break;
          case CV_32S: r = "32S"; break;
          case CV_32F: r = "32F"; break;
          case CV_64F: r = "64F"; break;
          default:     r = "User"; break;
     }
     r += "C";
     r += (chans+'0');
     return r;
}
std::string cvStrSize(const char* name, const cv::Mat& mat){
     std::string str = format("\'%s\' [%d, %d, %d, %s]", name, mat.cols, mat.rows, mat.channels(), cvtype2str(mat.type()).c_str());
     return str;
}
void cvinfo(const cv::Mat& mat, const char* label){
     double min, max;
     cv::minMaxLoc(mat, &min, &max);
     if(!label) printf("%s -- Limits = [%.3lf, %.3lf]\r\n",cvStrSize("Matrix",mat).c_str(), min, max);
     else printf("%s -- Limits = [%.3lf, %.3lf]\r\n",cvStrSize(label,mat).c_str(), min, max);
}
