#include "algorithms/vboats/vboats.h"
#include <math.h>       /* ceil, cos, sin, sqrt */
#include <chrono>
#include <string>

#include "base/definitions.h"
#include "utilities/utils.h"
#include "utilities/image_utils.h"
#include "algorithms/vboats/uvmap_utils.h"

#define VERBOSE_OBS_SEARCH false
#define VIZ_OBS_SEARCH false
#define VERBOSE_CONTOURS false
#define VIZ_CONTOURS false

using namespace std;
using namespace chrono;

Obstacle::Obstacle(vector<cv::Point> pts, vector<int> dBounds) :
     minXY(pts[0]), maxXY(pts[1]), dMin(dBounds[0]), dMax(dBounds[1])
{}

void Obstacle::update(bool depth_based, float cam_baseline, float cam_dscale, float* cam_focal,
     float* cam_principal_point, float dtype_gain, float aux_dist_factor, bool verbose)
{
     /** Default params for (Intel D415):
        --------------------------------
          focal     - 596.39, 596.39
          principle - 423.74, 242.01 (for 848 x 480)
          dscale    - 0.001
          baseline  - 0.014732
     */
     float fx = 596.39, fy = 596.39;
     if(cam_focal){
          fx = (float)cam_focal[0];
          fy = (float)cam_focal[1];
     }
     float ppx = 423.74, ppy = 242.01;
     if(cam_principal_point){
          ppx = (float)cam_principal_point[0];
          ppy = (float)cam_principal_point[1];
     }
     float dscale = 0.001;
     float baseline = 0.014732;
     if(cam_dscale != 0) dscale = cam_dscale;
     if(cam_baseline != 0) baseline = cam_baseline;

     float cvt_gain = 1.0;
     float aux_gain = 1.0;
     // float aux_gain = 1.35;
     if(dtype_gain != 0) cvt_gain = dtype_gain;
     if(aux_dist_factor != 0) aux_gain = aux_dist_factor;

     /** TODO: use this for more accurate  pixel information
          nonzero = umap.nonzero()
          nonzeroy = np.array(nonzero[0])
          nonzerox = np.array(nonzero[1])
          ymean = np.mean(ys)
          xmean = np.int(np.mean(nonzerox[good_inds]))
          dmean = np.mean(nonzeroy[good_inds])
     */
     float xmin = (float)this->minXY.x;
     float ymin = (float)this->minXY.y;
     float xmax = (float)this->maxXY.x;
     float ymax = (float)this->maxXY.y;
     float dmin = (float)this->dMin;
     float dmax = (float)this->dMax;

     float xmean = (xmin + xmax) / 2.0;
     float ymean = (ymin + ymax) / 2.0;
     float dmean = (dmin + dmax) / 2.0;

     float zgain, pz;
     if(depth_based){
          zgain = (65535.0 / 255.0)*dscale;
          pz = dmean * zgain;
     } else{
          float tmpDmean = dmean / cvt_gain;
          zgain = fx*baseline;
          pz = zgain / tmpDmean;
     }

     float x = ((xmean - ppx) * pz) / fx;
     float y = ((ymean - ppy) * pz) / fy;
     float dist = sqrt(x*x + pz*pz);
     double theta = atan2(x,pz);
     if(verbose) printf("Obstacle Relative Distance =  %.2fm --- Angle = %.2lf deg --- Position (X,Y,Z) = (%.2f, %.2f, %.2f) m \r\n", dist, theta*M_RAD2DEG, x,y, dmean);
     this->location.x = x;
     this->location.y = y;
     this->location.z = pz;
     this->distance = dist;
     this->angle = theta;
}

VBOATS::VBOATS(){}
VBOATS::~VBOATS(){}

void VBOATS::init(){}
void VBOATS::update(){}

void VBOATS::filter_disparity_vmap(const cv::Mat& input, cv::Mat* output, vector<float>* thresholds, bool verbose, bool debug, bool visualize){
     if(input.empty()) return;
     // if(verbose) printf("%s\r\n",cvStrSize("Vmap Filtering Input",input).c_str());
     vector<float> threshs;
     vector<float> default_threshs = {0.15,0.15,0.01,0.01};
     if(!thresholds) threshs = default_threshs;
     else threshs = (*thresholds);

     if(verbose){
          std::string threshStr = vector_str(threshs, ", ");
          printf("[INFO] VBOATS::filter_disparity_vmap() --- Using %d thresholds = [%s]\r\n", (int)threshs.size(), threshStr.c_str());
     }

     int err;
     cv::Mat filtered;
     cv::Mat imgCopy = input.clone();
     int nThreshs = threshs.size();

     int h = imgCopy.rows;
     int w = imgCopy.cols;
     int c = imgCopy.channels();
     // if(visualize){
     //      cv::namedWindow("input", cv::WINDOW_NORMAL );
     //      cv::imshow("input", imgCopy);
     //      cv::waitKey(0);
     // }

     double minVal, maxVal;
     cv::minMaxLoc(imgCopy, &minVal, &maxVal);

     float ratio = (float)(w) / 255.0;
     int nStrips = int(ceil(ratio * nThreshs)) - 1;
     int vMax = (int)maxVal;
     if(debug) printf("vMax, sizeRatio, nThreshs: %d, %.2f, %d\r\n", vMax, ratio, nStrips);

     /**
     int divSection = 3;
     int dSection = int(h/float(divSection));
     cv::Rect topHalfRoi = cv::Rect(0,0,w,dSection);
     cv::Rect botHalfRoi = cv::Rect(0,dSection,w,h-dSection);
     if(verbose){
          std::cout << "Top Portion ROI: " << topHalfRoi << std::endl;
          std::cout << "Bottom Portion ROI: " << botHalfRoi << std::endl;
     }

     cv::Mat topHalf = imgCopy(topHalfRoi);
     cv::Mat botHalf = imgCopy(botHalfRoi);
     if(visualize){
          // cv::namedWindow("top portion", cv::WINDOW_NORMAL );
          // cv::namedWindow("bottom portion", cv::WINDOW_NORMAL );
          // cv::imshow("top portion", topHalf);
          // cv::imshow("bottom portion", botHalf);
          // cv::waitKey(0);
     }

     cv::minMaxLoc(topHalf, &minVal, &maxVal);
     int tmpThresh = int(maxVal*0.65);
     if(verbose) printf("tops max, tmpThresh: %d, %d\r\n", int(maxVal), tmpThresh);

     cv::Mat topThreshed, prefiltered;
     threshold(topHalf, topThreshed, tmpThresh, 255, cv::THRESH_TOZERO);
     // threshold(topHalf, topThreshed, tmpThresh, 255, cv::THRESH_BINARY);
     topThreshed.copyTo(imgCopy(topHalfRoi));
     if(visualize){
          // cv::namedWindow("thresholded portion", cv::WINDOW_NORMAL );
          // cv::namedWindow("thresholded portion", cv::WINDOW_NORMAL );
          // cv::imshow("thresholded portion", topThreshed);
          // cv::imshow("input image w/ thresholded portion", imgCopy);
          // cv::waitKey(0);
     }
     */
     int idx = 0;
     cv::Scalar cvMean, cvStddev;
     std::vector<cv::Mat> strips;
     std::vector<cv::Mat> pStrips;
     err = strip_image(imgCopy, &strips, nStrips, true);
     if(err >= 0){
          for(cv::Mat strip : strips){
               cv::minMaxLoc(strip, &minVal, &maxVal);
               cv::meanStdDev(strip, cvMean, cvStddev);
               int tmpMax = (int) maxVal;
               double tmpMean = (double) cvMean[0];
               double tmpStd = (double) cvStddev[0];
               if(tmpMean == 0.0){
                    pStrips.push_back(strip.clone());
                    idx++;
                    continue;
               }

               if(debug) printf(" ---------- [Thresholding Strip %d] ---------- \r\n\tMax = %d, Mean = %.1lf, Std = %.1lf\r\n", idx,tmpMax,tmpMean,tmpStd);
               double maxValRatio = (double) vMax/255.0;
               double relRatio = (double)(tmpMax-tmpStd)/(double)vMax;
               double relRatio2 = (tmpMean)/(double)(tmpMax);
               if(debug) printf("\tRatios: %.3lf, %.3lf, %.3lf\r\n", maxValRatio, relRatio, relRatio2);

               double tmpGain;
               if(relRatio >= 0.4) tmpGain = relRatio + relRatio2;
               else tmpGain = 1.0 - (relRatio + relRatio2);

               int thresh = int(threshs[idx] * tmpGain * tmpMax);
               if(debug) printf("\tGain = %.2lf, Thresh = %d\r\n", tmpGain,thresh);

               cv::Mat tmpStrip;
               threshold(strip, tmpStrip, thresh, 255,cv::THRESH_TOZERO);
               pStrips.push_back(tmpStrip.clone());
               // if(visualize){
               //      cv::imshow("strip", strip);
               //      cv::imshow("thresholded strip", tmpStrip);
               //      cv::waitKey(0);
               // }
               idx++;
          }
          err = merge_strips(pStrips, &filtered);
     }else{
          if(verbose) printf("[WARNING] VBOATS::filter_disparity_vmap() --- Could not properly strip input vmap given the stripping parameters. Returning unfiltered vmap.\r\n");
          int wholeThresh = int(0.25*vMax);
          cv::threshold(imgCopy, filtered, wholeThresh, 255, cv::THRESH_TOZERO);
          //filtered = imgCopy.clone(); /** okay results */
     }
     if(visualize){
          cv::namedWindow("reconstructed thresholded vmap", cv::WINDOW_NORMAL );
          cv::imshow("reconstructed thresholded vmap", filtered);
          // cv::waitKey(0);
     }
     if(output) *output = filtered;
}
void VBOATS::filter_disparity_umap(const cv::Mat& input, cv::Mat* output, vector<float>* thresholds, bool verbose, bool debug, bool visualize){
     if(input.empty()) return;
     // if(verbose) printf("%s\r\n",cvStrSize("Umap Filtering Input",input).c_str());
     vector<float> threshs;
     vector<float> default_threshs = {0.25,0.15,0.35,0.35};
     if(!thresholds) threshs = default_threshs;
     else threshs = (*thresholds);

     if(verbose){
          std::string threshStr = vector_str(threshs, ", ");
          printf("[INFO] VBOATS::filter_disparity_umap() --- Using %d thresholds = [%s]\r\n", (int)threshs.size(), threshStr.c_str());
     }
     int err;
     cv::Mat filtered;
     cv::Mat imgCopy = input.clone();
     int nThreshs = threshs.size();

     int h = imgCopy.rows;
     int w = imgCopy.cols;
     int c = imgCopy.channels();
     // if(visualize){
     //      cv::namedWindow("input", cv::WINDOW_NORMAL );
     //      cv::imshow("input", imgCopy);
     //      cv::waitKey(0);
     // }

     double minVal, maxVal;
     cv::minMaxLoc(imgCopy, &minVal, &maxVal);

     float ratio = (float)(h) / 255.0;
     int uMax = (int)maxVal;
     int nStrips = int(ceil(ratio * nThreshs)) - 1;
     if(debug) printf("uMax, sizeRatio, nThreshs: %d, %.2f, %d\r\n", uMax, ratio, nStrips);

     cv::threshold(imgCopy, imgCopy, 8, 255, cv::THRESH_TOZERO);

     int idx = 0;
     cv::Scalar cvMean, cvStddev;
     std::vector<cv::Mat> strips;
     std::vector<cv::Mat> pStrips;
     err = strip_image(imgCopy, &strips, nStrips, false);
     if(err >= 0){
          for(cv::Mat strip : strips){
               cv::minMaxLoc(strip, &minVal, &maxVal);
               cv::meanStdDev(strip, cvMean, cvStddev);
               int tmpMax = (int) maxVal;
               double tmpMean = (double) cvMean[0];
               double tmpStd = (double) cvStddev[0];
               if(tmpMean == 0.0){
                    pStrips.push_back(strip.clone());
                    idx++;
                    continue;
               }

               if(debug) printf(" ---------- [Thresholding Strip %d] ---------- \r\n\tMax = %d, Mean = %.1lf, Std = %.1lf\r\n", idx,tmpMax,tmpMean,tmpStd);
               double maxValRatio = (double) uMax/255.0;
               double relRatio = (double)(tmpMax-tmpStd)/(double)uMax;
               double relRatio2 = (tmpMean)/(double)(tmpMax);
               if(debug) printf("\tRatios: %.3lf, %.3lf, %.3lf\r\n", maxValRatio, relRatio, relRatio2);

               double tmpGain;
               if(relRatio >= 0.4) tmpGain = relRatio + relRatio2;
               else tmpGain = 1.0 - (relRatio + relRatio2);

               int thresh = int(threshs[idx] * tmpGain * tmpMax);
               if(debug) printf("\tGain = %.2lf, Thresh = %d\r\n", tmpGain,thresh);

               cv::Mat tmpStrip;
               threshold(strip, tmpStrip, thresh, 255,cv::THRESH_TOZERO);
               pStrips.push_back(tmpStrip.clone());
               // if(visualize){
               //      cv::imshow("strip", strip);
               //      cv::imshow("thresholded strip", tmpStrip);
               //      cv::waitKey(0);
               // }
               idx++;
          }
          err = merge_strips(pStrips, &filtered, false);
     } else{
          if(verbose) printf("[WARNING] VBOATS::filter_disparity_umap() --- Could not properly strip input umap given the stripping parameters. Returning unfiltered umap.\r\n");
          int wholeThresh = int(0.25*uMax);
          cv::threshold(imgCopy, filtered, wholeThresh, 255, cv::THRESH_TOZERO);
          //filtered = imgCopy.clone(); /** okay results */
     }
     if(visualize){
          cv::namedWindow("reconstructed thresholded umap", cv::WINDOW_NORMAL );
          cv::imshow("reconstructed thresholded umap", filtered);
          // cv::waitKey(0);
     }

     if(output) *output = filtered;
}

bool VBOATS::is_ground_present(){return this->_is_ground_present;}
int VBOATS::find_ground_lines(const cv::Mat& vmap, cv::Mat* found_lines, int hough_thresh, bool verbose){
     if(vmap.empty()) return -1;
     cv::Mat _lines;
     cv::HoughLines(vmap, _lines, 1, CV_PI/180, hough_thresh);
     int n = _lines.size().height;
     int m = _lines.size().width;
     int c = _lines.channels();
     if(verbose) printf("[INFO] find_ground_lines() ---- Found %d, %d hough lines in image.\r\n", n, m);
     if(found_lines) *found_lines = _lines;
     return n;
}
int VBOATS::find_ground_lines(const cv::Mat& vmap, cv::Mat* rhos, cv::Mat* thetas, int hough_thresh, bool verbose){
     if(vmap.empty()) return -1;
     cv::Mat _lines;
     cv::HoughLines(vmap, _lines, 1, CV_PI/180, hough_thresh);
     int n = _lines.size().height;
     int m = _lines.size().width;
     int c = _lines.channels();
     if(verbose) printf("[INFO] find_ground_lines() ---- Found %d hough lines in image.\r\n", n);

     cv::Mat rs(n, 1, CV_32FC1);
     cv::Mat angs(n, 1, CV_32FC1);
     cv::Mat out[] = { rs, angs };
     int from_to[] = { 0,0, 1,1};
     cv::mixChannels( &_lines, 1, out, 2, from_to, 2 );

     if(rhos) *rhos = out[0];
     if(thetas) *thetas = out[1];
     return n;
}
int VBOATS::find_ground_lines(const cv::Mat& vmap, vector<cv::Vec2f>* found_lines, int hough_thresh, bool verbose){
     if(vmap.empty()) return -1;
     vector<cv::Vec2f> _lines;
     cv::HoughLines(vmap, _lines, 1, CV_PI/180, hough_thresh);
     int n = _lines.size();
     if(verbose) printf("[INFO] find_ground_lines() ---- Found %d hough lines in image.\r\n", n);
     if(found_lines) *found_lines = _lines;
     return n;
}
void VBOATS::get_hough_line_params(const float& rho, const float& theta, float* slope, int* intercept){
     float a = cos(theta),    b = sin(theta);
     float x0 = a*rho,        y0 = b*rho;
     int x1 = int(x0 - 1000.0 * b);
     int y1 = int(y0 + 1000.0 * a);
     int x2 = int(x0 + 1000.0 * b);
     int y2 = int(y0 - 1000.0 * a);
     float rise = (float)(y2 - y1);
     float run = (float)(x2 - x1);
     float _m = rise / run;
     int _intercept = y1 - int(_m * x1);
     if(slope) *slope = _m;
     if(intercept) *intercept = _intercept;
}
int VBOATS::estimate_ground_line(const vector<cv::Vec2f>& lines, float* best_slope,
     int* best_intercept, float* worst_slope, int* worst_intercept, double gnd_deadzone,
     double minDeg, double maxDeg, bool verbose, bool debug_timing)
{
     double t;
     if(debug_timing) t = (double)cv::getTickCount();

     float dAng = gnd_deadzone * M_DEG2RAD;
     float minAng = (minDeg * M_DEG2RAD) + (CV_PI/2.0);
     float maxAng = (maxDeg * M_DEG2RAD) + (CV_PI/2.0);

     vector<cv::Vec2f> buf;
     int nLines = lines.size();
     if(nLines <= 0) return -1;

     float sumRad = 0.0;
     float tmpRho, tmpAng;
     if(verbose) printf("[INFO] estimate_ground_line() ---- Filtering lines using limits (deg): Min=%.2f -- Max=%.2f\r\n", minAng*M_RAD2DEG, maxAng*M_RAD2DEG);
     for(int i = 0; i < nLines; i++){
          tmpRho = lines[i][0];
          tmpAng = lines[i][1];
          if(verbose){
               if(tmpAng > 0)
                    printf("[INFO] estimate_ground_line() ---- \tFiltering Line[%d] -- Angle=%.2f (deg)\r\n", i, tmpAng*M_RAD2DEG);
          }
          // if(verbose) printf("[INFO] estimate_ground_line() ---- \tFiltering Line[%d] -- Angle=%.2f (deg)\r\n", i, tmpAng*M_RAD2DEG);
          if(tmpAng >= minAng && tmpAng <= maxAng){
               sumRad += tmpAng;
               buf.push_back(cv::Vec2f(tmpRho, tmpAng));
          }
     }
     // if(verbose) printf("[INFO] estimate_ground_line() ---- using avg angle (%.2f deg) -> ground line (m = %.2f, b = %d) estimated found from %d / %d hough lines\r\n", avgAng*M_RAD2DEG, bestM, bestB, num,nLines);

     int num = 0, idx = 0;
     int tmpB = 0, bestB = 0, worstB = 0;
     float tmpM = 0.0, bestM = 0.0, worstM = 0.0;
     float avgAng = sumRad / float(buf.size());
     float minAvg = avgAng - dAng;
     float maxAvg = avgAng + dAng;
     if(verbose) printf("[INFO] estimate_ground_line() ---- Angle Limits (deg): Min=%.2f -- Avg=%.2f -- Max=%.2f\r\n", (minAvg*M_RAD2DEG)-90.0, (avgAng*M_RAD2DEG)-90.0, (maxAvg*M_RAD2DEG)-90.0);
     for(cv::Vec2f tmpLine : buf){
          idx++;
          if(verbose) printf("[INFO] estimate_ground_line() ---- Line[%d] -- Angle=%.2f (deg)\r\n", idx, (tmpLine[1]*M_RAD2DEG)-90.0);
          if(tmpLine[1] >= minAvg && tmpLine[1] <= maxAvg){
               get_hough_line_params(tmpLine[0],tmpLine[1], &tmpM, &tmpB);
               // if(verbose) printf("[INFO] estimate_ground_line() ---- \t line params: m = %.2f -- b = %d\r\n", tmpM, tmpB);
               if(tmpB > bestB){ bestM = tmpM; bestB = tmpB; }
               if(tmpB < worstB){ worstM = tmpM; worstB = tmpB; }
               num++;
          }
     }
     if(verbose) printf("[INFO] estimate_ground_line() ---- ground line best(m = %.2f, b = %d) -- worst(m = %.2f, b = %d) estimated found from %d / %d hough lines\r\n", bestM, bestB, worstM, worstB, num,nLines);
     if(debug_timing){
          t = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
          printf("[INFO] estimate_ground_line() ---- took %.4lf ms (%.2lf Hz)\r\n", t*1000.0, (1.0/t));
     }

     float slope, otherSlope;  int intercept, otherIntercept;
     bool bestValid = (bestB != 0) && (bestM != 0);
     bool worstValid = (worstB != 0) && (worstM != 0);
     if(!bestValid && worstValid){
          slope = worstM;
          intercept = worstB;
          otherSlope = bestM;
          otherIntercept = bestB;
     } else{
          slope = bestM;
          intercept = bestB;
          otherSlope = worstM;
          otherIntercept = worstB;
     }

     if(best_slope) *best_slope = slope;
     if(best_intercept) *best_intercept = intercept;
     if(worst_slope) *worst_slope = otherSlope;
     if(worst_intercept) *worst_intercept = otherIntercept;
     return num;
}
bool VBOATS::find_ground_line(const cv::Mat& vmap, float* best_slope, int* best_intercept,
     double minDeg, double maxDeg, int hough_thresh, double gnd_deadzone, bool verbose, bool debug_timing, bool visualize)
{
     if(vmap.empty()) return false;
     cv::Mat rs, angs;
     std::vector<cv::Vec2f> lines;

     double t;
     if(debug_timing) t = (double)cv::getTickCount();
     int nLines = find_ground_lines(vmap, &lines, hough_thresh);
     if(nLines <= 0) return false;

     float m, badm;  int b, badb;
     int nUsed = estimate_ground_line(lines, &m, &b, &badm, &badb, gnd_deadzone, minDeg, maxDeg);
     if(nUsed <= 0) return false;
     if(debug_timing){
          t = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
          printf("[INFO] find_ground_line() ---- took %.4lf ms (%.2lf Hz)\r\n", t*1000.0, (1.0/t));
     }

     if(best_slope) *best_slope = m;
     if(best_intercept) *best_intercept = b;

     if(visualize){
          cv::Mat display;
          cv::cvtColor(vmap, display, cv::COLOR_GRAY2BGR);
          int yf = int(vmap.cols * m) + b;

          cv::line(display, cv::Point(0, b), cv::Point(vmap.cols, yf), cv::Scalar(0,255,0), 2, cv::LINE_AA);
          // int yfbad = int(vmap.cols * badm) + badb;
          // cv::line(display, cv::Point(0, badb), cv::Point(vmap.cols, yfbad), cv::Scalar(0,0,255), 2, cv::LINE_AA);
          cv::imshow("Estimated Gnd", display);
          // cv::waitKey(0);
     }
     return true;
}

void VBOATS::extract_contour_bounds(const vector<cv::Point>& contour, vector<int>* xbounds, vector<int>* dbounds, bool verbose){
     // printf("extract_contour_bounds() --- %d\r\n", contour.size());
     cv::Rect tmpRect = cv::boundingRect(contour);
     // std::cout << "tmpRect: " << tmpRect << std::endl;
     vector<int> _xbounds = {tmpRect.x, tmpRect.x + tmpRect.width};
     vector<int> _dbounds = {tmpRect.y, tmpRect.y + tmpRect.height};

     if(xbounds) *xbounds = _xbounds;
     if(dbounds) *dbounds = _dbounds;
}
void VBOATS::find_contours(const cv::Mat& umap, vector<vector<cv::Point>>* found_contours,
     int filter_method, float min_threshold, int* offsets, float max_threshold,
     bool verbose, bool visualize, bool debug, bool debug_timing)
{
     if(umap.empty()) return;
     double t;
     if(debug_timing) t = (double)cv::getTickCount();

     cv::Mat image, drawing;
     vector<cv::Vec4i> hierarchy;
     vector<cv::Vec4i> filtHierarchy;
     vector< vector<cv::Point> > contours;
     vector<vector<cv::Point>> filtered_contours;
     if(umap.channels() <= 3) image = umap;
     else cv::cvtColor(umap, image, cv::COLOR_BGR2GRAY);
     if(visualize) drawing = cv::Mat::zeros(image.size(), CV_8UC3);

     int offx, offy;
     if(offsets){
          offx = offsets[0];
          offy = offsets[1];
     } else{ offx = 0; offy = 0; }
     if(debug) printf("offsets (x,y): %d, %d\r\n", offx, offy);

     cv::findContours(image, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(offx, offy));

     int nFiltCnts = 0;
     int nCnts = contours.size();
     if(debug) printf("Found %d contours\r\n",nCnts);
     double tmpVal, tmpArea, tmpLength, minVal = 1000, maxVal = 0;
     if(filter_method > 0){
          for(int i = 0; i<nCnts; i++){
               if(visualize) cv::drawContours(drawing, contours, i, cv::Scalar(255,0,0), 2, 8, hierarchy, 0, cv::Point(offx,offy));
               vector<cv::Point> contour = contours[i];
               cv::Vec4i hier = hierarchy[i];
               if(filter_method == 1){            /** Perimeter-based filtering */
                    tmpVal = cv::arcLength(contour,true);
                    if(tmpVal < min_threshold) continue;
                    if(max_threshold > 0){ if(tmpVal > max_threshold) continue; }
                    filtered_contours.push_back(contour);
                    filtHierarchy.push_back(hier);
                    if(tmpVal < minVal) minVal = tmpVal;
                    if(tmpVal > maxVal) maxVal = tmpVal;
               }else if(filter_method == 2){      /** Area-based filtering */
                    tmpVal = cv::contourArea(contour);
                    if(tmpVal < min_threshold) continue;
                    if(max_threshold > 0){ if(tmpVal > max_threshold) continue; }
                    filtered_contours.push_back(contour);
                    filtHierarchy.push_back(hier);
                    if(tmpVal < minVal) minVal = tmpVal;
                    if(tmpVal > maxVal) maxVal = tmpVal;
               }
          }
          if(debug) printf("Max val = %.2lf -- Min val = %.2lf\r\n",maxVal,minVal);
     } else filtered_contours = contours;
     nFiltCnts = filtered_contours.size();
     if(debug) printf("Filtered %d contours\r\n",nFiltCnts);

     if(debug_timing){
          t = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
          printf("[INFO] find_contours() ---- took %.4lf ms (%.2lf Hz) to find %d / %d contours\r\n", t*1000.0, (1.0/t), nFiltCnts,nCnts);
     }

     if(visualize){
          for(int i = 0; i<nFiltCnts; i++){ cv::drawContours(drawing, filtered_contours, i, cv::Scalar(0,255,0), 2, 8, filtHierarchy, 0, cv::Point(offx,offy)); }
          cv::namedWindow( "Contours", cv::WINDOW_AUTOSIZE ); cv::imshow( "Contours", drawing );
     }

     if(found_contours) *found_contours = filtered_contours;
}

int VBOATS::obstacle_search_disparity(const cv::Mat& vmap, const vector<int>& xLimits, vector<int>* yLimits,
     int* pixel_thresholds, int* window_size, std::vector<float> line_params, vector<cv::Rect>* obs_windows,
     bool verbose, bool visualize, bool debug, bool debug_timing)
{
     if(vmap.empty()) return -1;
     double t;
     if(debug_timing) t = (double)cv::getTickCount();

     int count = 0, nWindows = 0;
     bool flag_found_start = false, flag_hit_limits = false;
     bool flag_done = false, try_last_search = false, tried_last_resort = false;

     vector<int> _yLimits;
     vector<vector<cv::Point>> windows;
     vector<cv::Rect> obstacle_windows;
     cv::Mat img, display;
     if(vmap.channels() < 3) img = vmap;
     else cv::cvtColor(vmap, img, cv::COLOR_BGR2GRAY);
     if(visualize){
          display = vmap.clone();
          cv::cvtColor(display, display, cv::COLOR_GRAY2BGR);
          cv::namedWindow("search windows", cv::WINDOW_NORMAL );
     }

     int h = img.rows, w = img.cols;
     int xmin = xLimits.at(0);
     int xmax = xLimits.at(1);
     int pxlMin = 3, pxlMax = 30;
     if(pixel_thresholds){ pxlMin = pixel_thresholds[0], pxlMax = pixel_thresholds[1]; }

     int yk = 0, prev_yk = 0;
     int xmid = (int)((float)(xmax + xmin) / 2.0);
     int xk = (int) xmin;
     int dWy = 10, dWx = abs(xmid - xmin);
     int yf = h;

     if(!line_params.empty()){
          yf = (int)(xmid * line_params[0] + line_params[1]);
          // printf("Ground Line Coefficients - slope = %.2f, intercept = %d\r\n", line_params[0], (int)line_params[1]);
     }
     if(window_size){
          dWx = (int)((float) window_size[0] / 2.0);
          dWy = (int)((float) window_size[1] / 2.0);
     }
     if(dWx <= 2){
          if(xk <= 5) dWx = 1;
          else dWx = 2;
     }
     if(yk < 0) yk = 0;
     if(xk+2*dWx >= w){
          if(debug) printf("Problem xk = %d\r\n", xk);
          xk = (w-1)-2*dWx;
          if(debug) printf("New xk = %d\r\n", xk);
     }

     if(debug){
          printf("Input Image [%d x %d]\r\n", w,h);
          printf("X limits = [%d, %d] --- Y limits = [%d, %d] --- Pixel limits = [%d, %d] --- Window Size = [%d, %d]\r\n", xmin,xmax,yk, yf,pxlMin,pxlMax,dWx,dWy);
          printf("Object Search Window [%d x %d] -- Starting Location (%d, %d)\r\n", dWx,dWy,xk, yk);
     }
     cv::Rect searchRoi = cv::Rect(xk, yk, dWx*2, dWy*2);

     while(!flag_done){
          if(xk >= w){
               flag_hit_limits = true;
               if(debug) printf("[INFO] Reached max image width.\r\n");
          }
          if((yk >= yf) and (!line_params.empty())){
               flag_hit_limits = true;
               if(debug) printf("[INFO] Reached Estimated Ground line at y = %d.\r\n", yk);
          } else if(yk + dWy >= h){
               flag_hit_limits = true;
               if(debug) printf("[INFO] Reached max image height.\r\n");
          }
          if(flag_hit_limits) flag_done = true;
          if(count != 0){
               // Slide window from previousy found center (Clip at image edges)
               // Update vertical [Y] window edges
               if(yk - 2*dWy < 0) yk = 0;
               if(yk + 2*dWy >= h) yk = h - 2*dWy;

               // Update horizontal [X] window edges
               if(xk - 2*dWx < 0) xk = dWx;
               if(xk + 2*dWx >= w) xk = w - 2*dWx;
               searchRoi.y = yk;
          }

          if(debug) std::cout << "searchRoi: " << searchRoi << std::endl;
          cv::Mat roi = img(searchRoi);
          int nPxls = cv::countNonZero(roi);
          if(verbose) printf("Current Window [%d] ----- Center = (%d, %d) ----- %d of good pixels\r\n",count,xk,yk,nPxls);

          if(nPxls >= pxlMax){
               if(nWindows == 0){
                    _yLimits.push_back(yk);
                    if(visualize) cv::circle(display,cv::Point(xmid,yk),3,cv::Scalar(255,0,255),-1);
                    if(flag_hit_limits){
                         flag_done = false;
                         if(debug) printf("\tTrying Last Ditch Search...\r\n");
                    }
               } else{
                    _yLimits.push_back(yk+2*dWy);
                    try_last_search = false;
               }
               cv::Point pt1(xk,yk);
               cv::Point pt2(xk+2*dWx,yk+2*dWy);
               vector<cv::Point> tmpWindow = {pt1, pt2};
               windows.push_back(tmpWindow);
               nWindows++;
               prev_yk = yk;
               flag_found_start = true;
          } else if((nPxls <= pxlMin) && (flag_found_start)){
               if(debug) printf("Exiting - minimum threshold found...\r\n");
               // nWindows++;
               flag_done = true;
               _yLimits.push_back(yk);
               if(visualize) cv::circle(display,cv::Point(xmid,yk),3,cv::Scalar(255,0,255),-1);
          }
          obstacle_windows.push_back(searchRoi);
          if(visualize){
               cv::rectangle(display, searchRoi, cv::Scalar(0, 255, 255), 1);
               if(debug){
                    cv::imshow("search windows", display);
                    cv::waitKey(0);
               }
          }
          yk = yk + 2*dWy;
          count++;
     }
     if(debug){
          if(_yLimits.size() == 0) printf("[%d] Good Windows --- Ylimits = %d - %d\r\n", nWindows, 0, 0);
          else printf("[%d] Good Windows --- Ylimits = %d - %d\r\n", nWindows, _yLimits[0], _yLimits.back());
     }
     if(debug_timing){
          t = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
          printf("[INFO] obstacle_search_disparity() ---- took %.4lf ms (%.2lf Hz) to find %d good windows\r\n", t*1000.0, (1.0/t), nWindows);
     }

     if(!debug && visualize){
          cv::imshow("search windows", display);
          cv::waitKey(0);
     }
     if(yLimits) *yLimits = _yLimits;
     if(obs_windows) *obs_windows = obstacle_windows;
     return nWindows;
}
int VBOATS::find_obstacles_disparity(const cv::Mat& vmap, const vector<vector<cv::Point>>& contours,
     vector<Obstacle>* found_obstacles, std::vector<float> line_params, vector< vector<cv::Rect> >* obstacle_windows,
     bool verbose, bool debug_timing)
{
     if(vmap.empty()) return -1;
     int nObs = 0;
     bool gndPresent;
     vector<int> xLims;
     vector<int> dLims;
     vector<int> yLims;
     vector<Obstacle> obs;
     vector< vector<cv::Rect> > windows;
     double t = (double)cv::getTickCount();

     if(!line_params.empty()){
          if(verbose) printf("Ground Line Coefficients - slope = %.2f, intercept = %d\r\n", line_params[0], (int)line_params[1]);
          gndPresent = true;
     } else{
          if(verbose) printf("No Ground Found\r\n");
          gndPresent = false;
     }

     int nCnts = contours.size();
     for(int i = 0; i < nCnts; i++){
          vector<cv::Rect> obWindows;
          vector<cv::Point> contour = contours[i];
          extract_contour_bounds(contour,&xLims, &dLims);
          // int nWins = obstacle_search_disparity(vmap,dLims, &yLims, nullptr, nullptr, line_params, true);
          int nWins;
          if(obstacle_windows) nWins = obstacle_search_disparity(vmap,dLims, &yLims, nullptr, nullptr, line_params, &obWindows, VERBOSE_OBS_SEARCH, VIZ_OBS_SEARCH);
          else nWins = obstacle_search_disparity(vmap,dLims, &yLims, nullptr, nullptr, line_params, nullptr, VERBOSE_OBS_SEARCH, VIZ_OBS_SEARCH);

          if(nWins == 0) continue;
          if((yLims.size() <= 2) && (gndPresent)){
               if(verbose) printf("[INFO] Found obstacle with zero height. Skipping...\r\n");
               continue;
          } else if(yLims.size() <= 1){
               if(verbose) printf("[INFO] Found obstacle with zero height. Skipping...\r\n");
               continue;
          } else if(yLims[0] == yLims.back()){
               if(verbose) printf("[INFO] Found obstacle with zero height. Skipping...\r\n");
               continue;
          }else{
               if(verbose) printf("[INFO] Adding Obstacle (%d y limits)...\r\n", (int)yLims.size());
               // cv::Point(xLims[0],yLims[0]);
               // cv::Point(xLims[1],yLims.back());
               vector<cv::Point> pts = {cv::Point(xLims[0],yLims[0]), cv::Point(xLims[1],yLims.back())};
               obs.push_back(Obstacle(pts,dLims));
               if(obstacle_windows) windows.push_back(obWindows);
               nObs++;
          }
          if(verbose) printf(" --------------------------- \r\n");
     }
     if(debug_timing){
          t = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
          printf("[INFO] find_obstacles() ---- took %.4lf ms (%.2lf Hz) to find %d obstacles\r\n", t*1000.0, (1.0/t), (int)obs.size());
     }
     if(found_obstacles) *found_obstacles = obs;
     if(obstacle_windows) *obstacle_windows = windows;
     return nObs;
}

int VBOATS::pipeline_disparity(const cv::Mat& disparity, const cv::Mat& umap, const cv::Mat& vmap, vector<Obstacle>* obstacles, cv::Mat* uMorphElement, bool verbose, bool debug_timing, bool visualize){
     if(vmap.empty() || umap.empty() || disparity.empty()) return -1;
     double t0, tmpT,tmpT1, tmpDt, dt1, dt2, dt3;
     if(debug_timing) t0 = (double)cv::getTickCount();
     // vector<float> vthreshs = {0.15,0.15,0.01,0.01};
     // vector<float> vthreshs = {0.85,0.85,0.75,0.5};
     // vector<float> vthreshs = {0.45, 0.45,0.35,0.25};
     vector<float> vthreshs = {0.3, 0.3,0.25,0.4};

     // vector<float> uthreshs = {0.25,0.15,0.35,0.35};
     vector<float> uthreshs = {0.3,0.295,0.3,0.35};
     // vector<float> thresholds = {0.85,0.85,0.75,0.5};

     cv::Mat vTmp, uTmp, clone;
     cv::Mat vProcessed, uProcessed, sobelU,sobelV,tmpSobel, tmpAbsSobel;

     if(visualize) cv::cvtColor(disparity, clone, cv::COLOR_GRAY2BGR);

     if(debug_timing) tmpT1 = (double)cv::getTickCount();
     cv::cvtColor(umap, uTmp, cv::COLOR_GRAY2BGR);
     cv::rectangle(uTmp, cv::Point(0,0), cv::Point(umap.cols,3), cv::Scalar(0, 0, 0), -1);
     cv::cvtColor(uTmp, uTmp, cv::COLOR_BGR2GRAY);

     if(uMorphElement) cv::morphologyEx(uTmp, uTmp, cv::MORPH_CLOSE, *uMorphElement);
     cv::boxFilter(uTmp,uTmp,-1, cv::Size(2,2));
     filter_disparity_umap(uTmp, &uProcessed, &uthreshs);
     // cv::morphologyEx(uProcessed, uProcessed, cv::MORPH_CLOSE, element);
     if(debug_timing){
          tmpDt = ((double)cv::getTickCount() - tmpT1)/cv::getTickFrequency();
          printf("[INFO] umap prefiltering() ---- took %.4lf ms (%.2lf Hz)\r\n", tmpDt*1000.0, (1.0/tmpDt));
     }

     if(debug_timing) tmpT = (double)cv::getTickCount();
     cv::cvtColor(vmap, vTmp, cv::COLOR_GRAY2BGR);
     cv::rectangle(vTmp, cv::Point(0,0), cv::Point(3,vmap.rows), cv::Scalar(0, 0, 0), -1);
     cv::cvtColor(vTmp, vTmp, cv::COLOR_BGR2GRAY);
     filter_disparity_vmap(vTmp, &vProcessed, &vthreshs);

     double minVal, maxVal;
     cv::Sobel(vmap, tmpSobel, CV_64F, 0, 1, 3);
     cv::minMaxLoc(tmpSobel, &minVal, &maxVal);
     tmpSobel = tmpSobel * (255.0/maxVal);
     cv::convertScaleAbs(tmpSobel, tmpAbsSobel, 1, 0);
     threshold(tmpAbsSobel, sobelV, 30, 255, cv::THRESH_TOZERO);
     if(debug_timing){
          tmpDt = ((double)cv::getTickCount() - tmpT)/cv::getTickFrequency();
          printf("[INFO] vmap prefiltering() ---- took %.4lf ms (%.2lf Hz)\r\n", tmpDt*1000.0, (1.0/tmpDt));
     }

     if(debug_timing){
          dt1 = ((double)cv::getTickCount() - tmpT1)/cv::getTickFrequency();
          printf("[INFO] uv map filtering() ---- took %.4lf ms (%.2lf Hz)\r\n", dt1*1000.0, (1.0/dt1));
     }

     if(visualize){
          cv::Mat dvProcessed, duProcessed;
          cv::applyColorMap(uProcessed, duProcessed, cv::COLORMAP_JET);
          cv::applyColorMap(vProcessed, dvProcessed, cv::COLORMAP_JET);
          cv::namedWindow("processed vmap", cv::WINDOW_AUTOSIZE ); cv::imshow("processed vmap", dvProcessed);
          cv::namedWindow("processed umap", cv::WINDOW_AUTOSIZE ); cv::imshow("processed umap", duProcessed);
          // cv::waitKey(0);
     }

     if(debug_timing) tmpT = (double)cv::getTickCount();
     std::vector<float> line_params;
     float gndM; int gndB;
     bool gndPresent = find_ground_line(sobelV, &gndM,&gndB);
     // bool gndPresent = find_ground_line(vProcessed, &gndM,&gndB);
     if(gndPresent){
          line_params.push_back(gndM);
          line_params.push_back((float) gndB);
     }
     this->_is_ground_present = gndPresent;

     if(debug_timing){
          dt2 = ((double)cv::getTickCount() - tmpT)/cv::getTickFrequency();
          printf("[INFO] line finding() ---- took %.4lf ms (%.2lf Hz)\r\n", dt2*1000.0, (1.0/dt2));
     }

     if(debug_timing) tmpT = (double)cv::getTickCount();
     vector<vector<cv::Point>> contours;
     find_contours(uProcessed, &contours, 1, 100,nullptr,-1,VERBOSE_CONTOURS, VIZ_CONTOURS);
     // find_contours(uProcessed, &contours, 1, 30);
     // find_contours(sobelU, &contours);
     int n = 0;
     vector<Obstacle> _obstacles;
     int nObs = find_obstacles_disparity(vmap, contours, &_obstacles, line_params);
     // int nObs = find_obstacles_disparity(vProcessed, contours, &_obstacles, line_params);
     if(debug_timing){
          double dt2a = ((double)cv::getTickCount() - tmpT)/cv::getTickFrequency();
          printf("[INFO] obstacle finding() ---- took %.4lf ms (%.2lf Hz)\r\n", dt2a*1000.0, (1.0/dt2a));
     }

     if(debug_timing){
          tmpT = (double)cv::getTickCount();
          dt3 = ((double)cv::getTickCount() - tmpT)/cv::getTickFrequency();
          printf("[INFO] Obstacle detection --- took %.4lf ms (%.2lf Hz)\r\n", dt3*1000.0, (1.0/dt3));
     }
     if(obstacles) *obstacles = _obstacles;

     return nObs;
}
