#include <math.h>                  // For fabs, ceil

#include "algorithms/vboats/vboats.h"

using namespace std;

Vboats::Vboats(){
     // #ifdef WITH_CUDA
     // this->_cudaSreams = std::make_shared<std::vector<cv::cuda::Stream>>();
     // cv::cuda::Stream streamA, streamB, streamC, streamD;
     // this->_cudaSreams->push_back(streamA);
     // this->_cudaSreams->push_back(streamB);
     // this->_cudaSreams->push_back(streamC);
     // this->_cudaSreams->push_back(streamD);
     //
     // //Create Pinned Memory (PAGE_LOCKED) arrays
     // std::shared_ptr<std::vector<cv::cuda::HostMem >> srcMemArray = std::make_shared<std::vector<cv::cuda::HostMem >>();
     // std::shared_ptr<std::vector<cv::cuda::HostMem >> dstMemArray = std::make_shared<std::vector<cv::cuda::HostMem >>();
     //
     // //Create GpuMat arrays to use them on OpenCV CUDA Methods
     // std::shared_ptr<std::vector< cv::cuda::GpuMat >> gpuSrcArray = std::make_shared<std::vector<cv::cuda::GpuMat>>();
     // std::shared_ptr<std::vector< cv::cuda::GpuMat >> gpuDstArray = std::make_shared<std::vector<cv::cuda::GpuMat>>();
     //
     // //Create Output array for CPU Mat
     // std::shared_ptr<std::vector< cv::Mat >> outArray = std::make_shared<std::vector<cv::Mat>>();
     // for(int i=0; i<4; i++){
     //      //Define GPU Mats
     //      cv::cuda::GpuMat srcMat;
     //      cv::cuda::GpuMat dstMat;
     //      //Define CPU Mat
     //      cv::Mat outMat;
     //      //Initialize the Pinned Memory with input image
     //      cv::cuda::HostMem srcHostMem = cv::cuda::HostMem(srcHostImage, cv::cuda::HostMem::PAGE_LOCKED);
     //      //Initialize the output Pinned Memory with reference to output Mat
     //      cv::cuda::HostMem srcDstMem = cv::cuda::HostMem(outMat, cv::cuda::HostMem::PAGE_LOCKED);
     //
     //      //Add elements to each array.
     //      srcMemArray->push_back(srcHostMem);
     //      dstMemArray->push_back(srcDstMem);
     //
     //      gpuSrcArray->push_back(srcMat);
     //      gpuDstArray->push_back(dstMat);
     //      outArray->push_back(outMat);
     // }
     // #endif
}
Vboats::~Vboats(){}

void Vboats::init(){}
void Vboats::update(){}

cv::Mat Vboats::remove_umap_deadzones(const cv::Mat& umap){
     cv::Mat processed;
     if(umap.empty()) return processed;
     else processed = umap.clone();

     if(!this->_have_cam_info) return processed;
     int disparityFar = (int) ceil(this->_Tx / this->_hard_max_depth);

     // Create deadzone areas
     std::vector< std::vector<cv::Point> > deadzoneAreas;
     std::vector<cv::Point> deadzonePtsFar = {
          cv::Point(0, 0),
          cv::Point(umap.cols, 0),
          cv::Point(umap.cols, disparityFar),
          cv::Point(0, disparityFar),
          cv::Point(0, 0)
     };
     deadzoneAreas.push_back(deadzonePtsFar);

     // "blackout" deadzone regions
     cv::fillPoly(processed, deadzoneAreas, cv::Scalar(0));
     cv::boxFilter(processed, processed, -1, cv::Size(2,2));
     return processed;
}
cv::Mat Vboats::remove_vmap_deadzones(const cv::Mat& vmap){
     cv::Mat processed;
     if(vmap.empty()) return processed;
     else processed = vmap.clone();

     if(!this->_have_cam_info) return processed;
     int disparityFar = (int) ceil(this->_Tx / this->_hard_max_depth);
     int disparityNear = (int) ceil(this->_Tx / this->_hard_min_depth);

     vector<vector<cv::Point>> deadzoneAreas;
     vector<cv::Point> deadzonePtsFar = {
          cv::Point(0, 0),
          cv::Point(0, vmap.rows),
          cv::Point(disparityFar, vmap.rows),
          cv::Point(disparityFar, 0),
          cv::Point(0, 0)
     };
     deadzoneAreas.push_back(deadzonePtsFar);

     vector<cv::Point> deadzonePtsNear = {
          cv::Point(vmap.cols, 0),
          cv::Point(vmap.cols, vmap.rows),
          cv::Point(disparityNear, vmap.rows),
          cv::Point(disparityNear, 0),
          cv::Point(vmap.cols, 0)
     };
     deadzoneAreas.push_back(deadzonePtsNear);

     // "blackout" deadzone regions
     cv::fillPoly(processed, deadzoneAreas, cv::Scalar(0));
     return processed;
}

cv::Mat Vboats::generate_disparity_from_depth(const cv::Mat& depth){
     cv::Mat output;
     if(depth.empty()) return output;
     if(!this->_have_cam_info) return output;

     cv::Mat image;
     float gain = this->_depth2disparityFactor;
     if(depth.type() == CV_16UC1){
          depth.convertTo(image, CV_32F, this->_depth_scale);
          gain = gain * this->_depth_scale;
     } else if(depth.type() != CV_32F) depth.convertTo(image, CV_32F);
     else image = depth.clone();

     if(this->_debug_disparity_gen){
          std::string lbl1 = format("[DEBUG] %s --- DepthScale = %.4f && Input Depth = ", this->classLbl.c_str(), gain);
          std::string lbl2 = format("[DEBUG] %s --- Depth Image Before ForEach Operation = ", this->classLbl.c_str());
          cvinfo(depth, lbl1.c_str());
          cvinfo(image, lbl2.c_str());
     }

     float xMinVal, xMaxVal;
     if(this->_flip_object_dimension_x_limits){
          xMaxVal = (float) this->_object_min_dimension_x;
          xMinVal = (float) this->_object_max_dimension_x;
     } else{
          xMinVal = (float) this->_object_min_dimension_x;
          xMaxVal = (float) this->_object_max_dimension_x;
     }

     float yMinVal, yMaxVal;
     if(this->_flip_object_dimension_y_limits){
          yMaxVal = (float) this->_object_min_dimension_y;
          yMinVal = (float) this->_object_max_dimension_y;
     } else{
          yMinVal = (float) this->_object_min_dimension_y;
          yMaxVal = (float) this->_object_max_dimension_y;
     }

     ForEachSaturateDepthLimits<float> d2dconverter(gain,
          (float) this->_hard_min_depth, (float) this->_hard_max_depth,
          (float) this->_fx, (float) this->_fy, (float) this->_px, (float) this->_py,
          xMinVal, xMaxVal, yMinVal, yMaxVal
     );
     // ForEachDepthConverter<float> d2dconverter(gain, (float) this->_hard_min_depth, (float) this->_hard_max_depth);

     image.forEach<float>(d2dconverter);

     if(this->_debug_disparity_gen){
          std::string lbl1 = format("[DEBUG] %s --- Depth Image After ForEach Operation = ", this->classLbl.c_str());
          cvinfo(image, lbl1.c_str());
     }

     if(image.type() != CV_8UC1){
          double minVal, maxVal;
          cv::minMaxLoc(image, &minVal, &maxVal);
          double normalize_gain = 255.0 / maxVal;
          this->_depth_deproject_gain = 1.0 / normalize_gain;
          image.convertTo(output, CV_8UC1, normalize_gain);
     } else{
          this->_depth_deproject_gain = 1.0;
          output = image.clone();
     }
     if(this->_debug_disparity_gen){
          std::string lbl1 = format("[DEBUG] %s --- Function Output = ", this->classLbl.c_str());
          cvinfo(output, lbl1.c_str());
     }
     return output.clone();
}
cloudxyz_t::Ptr Vboats::generate_pointcloud_from_depth(const cv::Mat& depth, bool debug_timing){
     cloudxyz_t::Ptr tmpCloud(new cloudxyz_t);
     // Return early with empty cloud object if input depth image is empty
     // or the necessary camera information was never recevied
     if( (depth.empty()) || (!this->_have_cam_info) ) return tmpCloud;
     // Convert input image to proper data type needed for cloud generation
     cv::Mat tmpDepth = depth.clone();
     if(depth.type() == CV_16UC1) tmpDepth.convertTo(tmpDepth, CV_32F, this->_depth_scale);
     else if(depth.type() != CV_32F) tmpDepth.convertTo(tmpDepth, CV_32F);

     double t;
     if(debug_timing) t = (double)cv::getTickCount();
     // Loop through depth image and generate cloud XYZ points using depth information
     float* ptrP;
     pcl::PointXYZ pt;
     for(int y = 0; y < tmpDepth.rows; y+=4){
          ptrP = tmpDepth.ptr<float>(y);
          for(int x = 0; x < tmpDepth.cols; x+=4){
               float depthVal = (float) ptrP[x];
               if(depthVal > 0){
                    pt.x = ((float)x - this->_px) * depthVal / this->_fx;
                    pt.y = ((float)y - this->_py) * depthVal / this->_fy;
                    pt.z = depthVal;
                    tmpCloud->points.push_back(pt);
               }
          }
     }
     tmpCloud->height = 1;
     tmpCloud->width = tmpCloud->points.size();
     if(debug_timing){
          t = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
          printf("[INFO] %s::generate_pointcloud_from_depth() ---- took %.4lf ms (%.2lf Hz) to generate a pointcloud from a depth image\r\n", this->classLbl.c_str(), t*1000.0, (1.0/t));
     }
     return tmpCloud;
}

int Vboats::process(const cv::Mat& depth, cv::Mat* filtered_input,
     std::vector<Obstacle>* found_obstacles, std::vector<float>* line_coefficients,
     cv::Mat* disparity_output, cv::Mat* umap_output, cv::Mat* vmap_output,
     cv::Mat* umap_input, cv::Mat* vmap_input,
     bool verbose_obstacles, bool debug
){
     double t, tUVmapProc, tTotal, dt;
     // <custom-fold Receive and Check Data Input Sources
     if(depth.empty()){
          printf("[WARNING] %s::process() --- Depth input is empty.\r\n", this->classLbl.c_str());
          return -1;
     }
     if(!this->_have_cam_info){
          printf("[WARNING] %s::process() --- Camera Information Unknown.\r\n", this->classLbl.c_str());
          return -2;
     }

     cv::Mat depthRaw = depth.clone();

     if(this->_debug_process_timings){
          t = (double)cv::getTickCount();
          tTotal = t;
     }
     cv::Mat disparity = this->generate_disparity_from_depth(depthRaw);
     if(disparity.empty()){
          printf("[WARNING] %s::process() --- Disparity input is empty.\r\n", this->classLbl.c_str());
          return -3;
     }
     cv::Mat disparityRaw = disparity.clone();

     if(this->_debug_process_timings){
          dt = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
          printf("[DEBUG] %s::process() --- --- disparity generation took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
          t = (double)cv::getTickCount();
     }
     cv::Mat depthInput, disparityInput;
     double correctionAngle = this->get_correction_angle(true, this->_flip_correction_angle_sign);
     if(this->_do_angle_correction){
          cv::Mat warpedDepth = rotate_image(depthRaw, correctionAngle);
          cv::Mat warpeddDisparity = rotate_image(disparityRaw, correctionAngle);

          if(!warpedDepth.empty() && !warpeddDisparity.empty()){
               depthInput = warpedDepth.clone();
               disparityInput = warpeddDisparity.clone();
               this->_angle_correction_performed = true;
          } else{
               depthInput = depthRaw;
               disparityInput = disparityRaw;
               this->_angle_correction_performed = false;
          }
     } else{
          depthInput = depthRaw;
          disparityInput = disparityRaw;
          this->_angle_correction_performed = false;
     }
     if(disparity_output) *disparity_output = disparityInput.clone();
     if(this->_debug_process_timings){
          dt = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
          printf("[DEBUG] %s::process() --- --- angle correction took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
          t = (double)cv::getTickCount();
     }
     // </custom-fold>

     cv::Mat umap, vmap;
     genUVMapThreaded(disparityInput, &umap, &vmap, 2.0);
     if(umap.empty()){
          printf("[WARNING] %s::process() --- Umap input is empty.\r\n", this->classLbl.c_str());
          return -4;
     }
     if(vmap.empty()){
          printf("[WARNING] %s::process() --- Vmap input is empty.\r\n", this->classLbl.c_str());
          return -5;
     }
     cv::Mat umapRaw = umap.clone();
     cv::Mat vmapRaw = vmap.clone();
     if(umap_input || this->processingDebugger.visualize_umap_raw){
          this->processingDebugger.set_umap_raw(umapRaw);
          if(umap_input) *umap_input = umapRaw.clone();
     }
     if(vmap_input || this->processingDebugger.visualize_vmap_raw){
          this->processingDebugger.set_vmap_raw(vmapRaw);
          if(vmap_input) *vmap_input = vmapRaw.clone();
     }
     if(this->_debug_process_timings){
          dt = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
          printf("[DEBUG] %s::process() --- --- uv-map generation took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
          t = (double)cv::getTickCount();
          tUVmapProc = t;
     }

     // <custom-fold UV-Map Pre-processing
     // Pre-process Umap
     cv::Mat uProcessed = this->remove_umap_deadzones(umapRaw);
     if(this->_umapFiltMeth == SOBELIZED_METHOD){
          uProcessed = preprocess_umap_sobelized(uProcessed,
               this->umapParams.sobel_thresh_pre_sobel,
               this->umapParams.sobel_thresh_sobel_preprocess,
               this->umapParams.sobel_thresh_sobel_postprocess,
               this->umapParams.sobel_dilate_size, this->umapParams.sobel_blur_size,
               this->umapParams.sobel_kernel_multipliers,
               &this->processingDebugger
          );
     } else if(this->_umapFiltMeth == STRIPPING_METHOD) uProcessed = preprocess_umap_stripping(uProcessed, &this->umapParams.stripping_threshs);

     if(this->_debug_process_timings){
          dt = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
          printf("[DEBUG] %s::process() --- --- umap processing took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
          t = (double)cv::getTickCount();
     }

     // Pre-process Vmap
     cv::Mat preprocessedVmap = this->remove_vmap_deadzones(vmapRaw);
     if(this->_vmapFiltMeth == SOBELIZED_METHOD){
          preprocessedVmap = preprocess_vmap_sobelized(preprocessedVmap,
               this->vmapParams.sobel_preprocessing_thresh_sobel,
               this->vmapParams.sobel_preprocessing_blur_size,
               this->vmapParams.sobel_kernel_multipliers
          );
     } else if(this->_vmapFiltMeth == STRIPPING_METHOD) preprocessedVmap = preprocess_vmap_stripping(preprocessedVmap, &this->vmapParams.stripping_threshs);
     this->processingDebugger.set_vmap_sobelized_preprocessed(preprocessedVmap);

     // Finish V-map processing starting from the pre-processed vmap
     cv::Mat vProcessed = preprocessedVmap.clone();
     if(this->_vmapFiltMeth == SOBELIZED_METHOD){
          vProcessed = postprocess_vmap_sobelized(vmapRaw, preprocessedVmap,
               this->vmapParams.sobel_postprocessing_thresh_prefiltering,
               this->vmapParams.sobel_postprocessing_thresh_postfiltering,
               this->vmapParams.sobel_postprocessing_blur_size,
               this->vmapParams.sobel_kernel_multipliers,
               &this->processingDebugger
          );
     }

     if(this->_debug_process_timings){
          dt = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
          printf("[DEBUG] %s::process() --- --- vmap processing took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
          t = (double)cv::getTickCount();
     }

     // Extract ground line parameters (if ground is present)
     std::vector<float> line_params;
     bool gndPresent = find_ground_line(preprocessedVmap, &line_params,
          this->vmapParams.gnd_line_search_min_deg,
          this->vmapParams.gnd_line_search_max_deg,
          this->vmapParams.gnd_line_search_deadzone,
          this->vmapParams.gnd_line_search_hough_thresh,
          false, this->_debug_process_timings
     );

     if(this->_debug_process_timings){
          dt = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
          printf("[DEBUG] %s::process() --- --- ground line estimation took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
          t = (double)cv::getTickCount();
     }

     // Find contours in Umap needed later for obstacle filtering
     vector<vector<cv::Point>> filtered_contours;
     find_contours(uProcessed, &filtered_contours, (int) this->_contourFiltMeth,
          this->umapParams.contour_filtering_thresh_min,
          this->umapParams.contour_filtering_thresh_max,
          &this->umapParams.contour_filtering_offset,
          &this->processingDebugger.filtered_umap_hierarchies,  // filtered contours visualization
          nullptr,  // all_contours visualization
          nullptr   // all_hierarchies visualization
     );
     this->processingDebugger.set_filtered_umap_contours(filtered_contours);
     // </custom-fold>

     if(this->_debug_process_timings){
          double tmpT = (double)cv::getTickCount();
          dt = (tmpT - t)/cv::getTickFrequency();
          printf("[DEBUG] %s::process() --- --- umap contour filtering took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
          dt = (tmpT - tUVmapProc)/cv::getTickFrequency();
          printf("[DEBUG] %s::process() --- --- Overall UV-map processing block took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
          t = (double)cv::getTickCount();
     }

     float delta_slope   = 0;
     int delta_intercept = 0;
     if(!line_params.empty()){
          float cur_gnd_line_slope       = (float) line_params[0];
          int cur_gnd_line_intercept     = (int) line_params[1];
          delta_slope                    = fabs(cur_gnd_line_slope - this->_prev_gnd_line_slope);
          delta_intercept                = abs(cur_gnd_line_intercept - this->_prev_gnd_line_intercept);
          this->_prev_gnd_line_slope     = cur_gnd_line_slope;
          this->_prev_gnd_line_intercept = cur_gnd_line_intercept;
          if(this->_check_gnd_line_noise){
               if(delta_slope > this->_delta_gnd_line_slope_thresh){
                    // Override ground line detection flag
                    gndPresent = false;
                    printf("[INFO] %s::process() --- Noisy Ground Line Detected, slope difference (%.3f) > %.3f.\r\n",
                         this->classLbl.c_str(), delta_slope, this->_delta_gnd_line_slope_thresh
                    );
               } else if(delta_intercept > this->_delta_gnd_line_intercept_thresh){
                    // Override ground line detection flag
                    gndPresent = false;
                    printf("[INFO] %s::process() --- Noisy Ground Line Detected, intercept difference (%.3f) > %.3f.\r\n",
                         this->classLbl.c_str(), delta_intercept, this->_delta_gnd_line_intercept_thresh
                    );
               }
          }
     }
     if(!gndPresent){
          printf("[INFO] %s::process() --- Unable to detect Ground Line.\r\n", this->classLbl.c_str());
          // Ensure any ground line dependent functions get notified on no ground line
          // via null coefficients
          line_params = std::vector<float>{};
          this->processingDebugger.set_gnd_line_coefficients(std::vector<float>{});
          if(line_coefficients) *line_coefficients = std::vector<float>{};
     } else{
          this->processingDebugger.set_gnd_line_coefficients(line_params);
          if(line_coefficients) *line_coefficients = std::vector<float>(line_params.begin(), line_params.end());
     }
     if(umap_output || this->processingDebugger.visualize_umap_final){
          this->processingDebugger.set_umap_processed(uProcessed);
          if(umap_output) *umap_output = uProcessed.clone();
     }
     if(vmap_output || this->processingDebugger.visualize_vmap_final){
          this->processingDebugger.set_vmap_processed(vProcessed);
          if(vmap_output) *vmap_output = vProcessed.clone();
     }
     // <custom-fold UV-Map Post-processing

     // Obstacle data extraction
     int nObs = 0;
     std::vector<Obstacle> obstacles_;
     std::vector< std::vector<cv::Rect> > obstacleRegions;
     if(this->_debug_process_timings) t = (double)cv::getTickCount();
     if(this->_do_obstacle_data_extraction){
          nObs = find_obstacles_disparity(vProcessed, filtered_contours, line_params, &obstacles_,
               &obstacleRegions    // obstacle regions visualization
          );

          if(this->_debug_process_timings){
               dt = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
               printf("[DEBUG] %s::process() --- --- finding individual obstacles took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
               t = (double)cv::getTickCount();
          }
     }
     this->processingDebugger.set_vmap_object_search_regions(obstacleRegions);

     // Filter the original depth image using all the useful data encoded within
     // the resulting processed UV-Maps
     if(this->_debug_process_timings) t = (double)cv::getTickCount();
     cv::Mat filtered_image;
     int err = filter_depth_using_ground_line(depthInput, disparityInput,
          vProcessed, line_params, &filtered_image,
          this->vmapParams.depth_filtering_gnd_line_intercept_offset,
          &this->processingDebugger,   // keep_mask visualization
          false
     );
     // int err = filter_depth_using_object_candidate_regions(depthInput, disparityInput,
     //      vProcessed, filtered_contours, &filtered_image, line_params,
     //      this->vmapParams.depth_filtering_gnd_line_intercept_offset, &this->processingDebugger
     // );
     // this->processingDebugger.set_gnd_line_intercept_offset(this->vmapParams.depth_filtering_gnd_line_intercept_offset);

     if(this->_debug_process_timings){
          dt = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
          printf("[DEBUG] %s::process() --- --- ground line based depth filtering took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
          t = (double)cv::getTickCount();
     }

     cv::Mat filtered_depth;
     err = filter_depth_using_object_candidate_regions(filtered_image, disparityInput,
          vProcessed, filtered_contours, &filtered_depth, line_params,
          this->vmapParams.depth_filtering_gnd_line_intercept_offset, &this->processingDebugger
     );
     this->processingDebugger.set_gnd_line_intercept_offset(this->vmapParams.depth_filtering_gnd_line_intercept_offset);
     // err = filter_depth_using_ground_line(filtered_image, disparityInput,
     //      vProcessed, line_params, &filtered_depth,
     //      this->vmapParams.depth_filtering_gnd_line_intercept_offset,
     //      &this->processingDebugger,   // keep_mask visualization
     //      false
     // );

     if(this->_debug_process_timings){
          dt = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
          printf("[DEBUG] %s::process() --- --- obstacle candidate based depth filtering took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
          t = (double)cv::getTickCount();
     }

     // Attempt to remove any noise (speckles) from the resulting filtered depth image
     cv::Mat final_depth;
     if(this->_denoise_filtered_depth){
          cv::Mat morphElement = cv::getStructuringElement(cv::MORPH_ELLIPSE,
               cv::Size(this->_filtered_depth_denoising_size, this->_filtered_depth_denoising_size)
          );
          cv::Mat morphedDepth;
          cv::morphologyEx(filtered_depth, morphedDepth, cv::MORPH_OPEN, morphElement);
          final_depth = morphedDepth.clone();
     } else final_depth = filtered_depth.clone();
     if(this->_do_angle_correction && this->_angle_correction_performed) final_depth = rotate_image(final_depth, -correctionAngle);

     if(this->_debug_process_timings){
          dt = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
          printf("[DEBUG] %s::process() --- --- final filtered depth corrections took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
          t = (double)cv::getTickCount();
     }

     std::vector<Obstacle> obstacles_output;
     if( (this->_do_obstacle_data_extraction) && (!obstacles_.empty()) ){
          for(Obstacle obj : obstacles_){
               obj.update(false, this->_baseline, this->_depth_scale,
                    std::vector<float>{this->_fx, this->_fy},
                    std::vector<float>{this->_px, this->_py},
                    this->_depth_deproject_gain, 1.0, verbose_obstacles
               );
               obstacles_output.push_back(obj);
          }
          if(this->_debug_process_timings){
               dt = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
               printf("[DEBUG] %s::process() --- --- individual obstacle geometric data calculations took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
          }
     } else{ obstacles_output.reserve(nObs); obstacles_output.assign(obstacles_.begin(), obstacles_.end()); }

     if(this->_debug_process_timings){
          dt = ((double)cv::getTickCount() - tUVmapProc)/cv::getTickFrequency();
          printf("[DEBUG] %s::process() --- --- Entire processing pipeline took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
     }

     // Return Output images if requested before visualization
     if(found_obstacles) *found_obstacles = std::vector<Obstacle>(obstacles_output.begin(), obstacles_output.end());
     if(filtered_input) *filtered_input = final_depth.clone();
     return (int) obstacles_.size();
}


// std::shared_ptr<std::vector<cv::Mat>> Vboats::computeArray(
//      std::shared_ptr<std::vector< cv::cuda::HostMem >> srcMemArray,
//      std::shared_ptr<std::vector< cv::cuda::HostMem >> dstMemArray,
//      std::shared_ptr<std::vector< cv::cuda::GpuMat >> gpuSrcArray,
//      std::shared_ptr<std::vector< cv::cuda::GpuMat >> gpuDstArray,
//      std::shared_ptr<std::vector< cv::Mat >> outArray,
//      std::shared_ptr<std::vector< cv::cuda::Stream >> streamsArray)
// {
//      //Define test target size
//      cv::Size rSize(256, 256);
//      //Compute for each input image with async calls
//      for(int i=0; i<4; i++){
//           //Upload Input Pinned Memory to GPU Mat
//           (*gpuSrcArray)[i].upload((*srcMemArray)[i], (*streamsArray)[i]);
//           //Use the CUDA Kernel Method
//           cv::cuda::resize((*gpuSrcArray)[i], (*gpuDstArray)[i], rSize, 0, 0, cv::INTER_AREA, (*streamsArray)[i]);
//           //Download result to Output Pinned Memory
//           (*gpuDstArray)[i].download((*dstMemArray)[i],(*streamsArray)[i]);
//           //Obtain data back to CPU Memory
//           (*outArray)[i] = (*dstMemArray)[i].createMatHeader();
//      }
//      //All previous calls are non-blocking therefore
//      //wait for each stream completetion
//      (*streamsArray)[0].waitForCompletion();
//      (*streamsArray)[1].waitForCompletion();
//      (*streamsArray)[2].waitForCompletion();
//      (*streamsArray)[3].waitForCompletion();
//
//      return outArray;
// }

int Vboats::process_w_cuda(const cv::Mat& depth, cv::Mat* filtered_input,
     std::vector<Obstacle>* found_obstacles, std::vector<float>* line_coefficients,
     cv::Mat* disparity_output, cv::Mat* umap_output, cv::Mat* vmap_output,
     cv::Mat* umap_input, cv::Mat* vmap_input,
     bool verbose_obstacles, bool debug
){
     // #ifdef WITH_CUDA
     double t, tUVmapProc, tTotal, dt;
     // <custom-fold Receive and Check Data Input Sources
     if(depth.empty()){
          printf("[WARNING] %s::process() --- Depth input is empty.\r\n", this->classLbl.c_str());
          return -1;
     }
     if(!this->_have_cam_info){
          printf("[WARNING] %s::process_w_cuda() --- Camera Information Unknown.\r\n", this->classLbl.c_str());
          return -2;
     }

     cv::Mat depthRaw = depth.clone();

     if(this->_debug_process_timings){
          t = (double)cv::getTickCount();
          tTotal = t;
     }
     cv::Mat disparity = this->generate_disparity_from_depth(depthRaw);
     if(disparity.empty()){
          printf("[WARNING] %s::process_w_cuda() --- Disparity input is empty.\r\n", this->classLbl.c_str());
          return -3;
     }
     cv::Mat disparityRaw = disparity.clone();

     if(this->_debug_process_timings){
          dt = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
          printf("[DEBUG] %s::process_w_cuda() --- --- disparity generation took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
          t = (double)cv::getTickCount();
     }
     cv::Mat depthInput, disparityInput;
     double correctionAngle = this->get_correction_angle(true, this->_flip_correction_angle_sign);
     if(this->_do_angle_correction){
          cv::Mat warpedDepth = rotate_image(depthRaw, correctionAngle);
          cv::Mat warpeddDisparity = rotate_image(disparityRaw, correctionAngle);

          if(!warpedDepth.empty() && !warpeddDisparity.empty()){
               depthInput = warpedDepth.clone();
               disparityInput = warpeddDisparity.clone();
               this->_angle_correction_performed = true;
          } else{
               depthInput = depthRaw;
               disparityInput = disparityRaw;
               this->_angle_correction_performed = false;
          }
     } else{
          depthInput = depthRaw;
          disparityInput = disparityRaw;
          this->_angle_correction_performed = false;
     }
     if(disparity_output) *disparity_output = disparityInput.clone();
     if(this->_debug_process_timings){
          dt = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
          printf("[DEBUG] %s::process_w_cuda() --- --- angle correction took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
          t = (double)cv::getTickCount();
     }
     // </custom-fold>

     cv::Mat umap, vmap;
     genUVMapThreaded(disparityInput, &umap, &vmap, 4.0);
     if(umap.empty()){
          printf("[WARNING] %s::process_w_cuda() --- Umap input is empty.\r\n", this->classLbl.c_str());
          return -4;
     }
     if(vmap.empty()){
          printf("[WARNING] %s::process_w_cuda() --- Vmap input is empty.\r\n", this->classLbl.c_str());
          return -5;
     }
     cv::Mat umapRaw = umap.clone();
     cv::Mat vmapRaw = vmap.clone();
     if(umap_input || this->processingDebugger.visualize_umap_raw){
          this->processingDebugger.set_umap_raw(umapRaw);
          if(umap_input) *umap_input = umapRaw.clone();
     }
     if(vmap_input || this->processingDebugger.visualize_vmap_raw){
          this->processingDebugger.set_vmap_raw(vmapRaw);
          if(vmap_input) *vmap_input = vmapRaw.clone();
     }
     if(this->_debug_process_timings){
          dt = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
          printf("[DEBUG] %s::process_w_cuda() --- --- uv-map generation took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
          t = (double)cv::getTickCount();
          tUVmapProc = t;
     }

     // <custom-fold UV-Map Pre-processing
     // Pre-process Umap
     cv::Mat umapInput = this->remove_umap_deadzones(umapRaw);
     cv::Mat vmapInput = this->remove_vmap_deadzones(vmapRaw);

     process_uvmaps_sobelized_cuda(umapInput, vmapInput,
          this->umapParams, this->vmapParams, this->umapBuffer, this->vmapBuffer
     );

     cv::Mat uProcessed = this->umapBuffer.processed;
     // cv::Mat preprocessedVmap = this->vmapBuffer.preprocessed.clone();
     cv::Mat vProcessed = this->vmapBuffer.postprocessed.clone();
     std::vector<cv::Vec2f> found_lines = std::vector<cv::Vec2f>{this->vmapBuffer.lines.begin(), this->vmapBuffer.lines.end()};
     // int nFound = (int) found_lines.size();

     if(this->_debug_process_timings){
          dt = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
          printf("[DEBUG] %s::process_w_cuda() --- --- uvmap processing took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
          t = (double)cv::getTickCount();
     }

     std::vector<float> line_params;
     bool gndPresent = true;
     if(found_lines.empty()) gndPresent = false;
     else{
          float m, badm;
          int b, badb;
          int nUsed = estimate_ground_line_coefficients(found_lines, &m, &b, &badm, &badb,
               this->vmapParams.gnd_line_search_deadzone,
               this->vmapParams.gnd_line_search_min_deg,
               this->vmapParams.gnd_line_search_max_deg
          );
          if(nUsed <= 0) gndPresent = false;
          else line_params = std::vector<float>{m, (float) b};
     }

     if(this->_debug_process_timings){
          dt = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
          printf("[DEBUG] %s::process_w_cuda() --- --- ground line estimation took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
          t = (double)cv::getTickCount();
     }

     // Find contours in Umap needed later for obstacle filtering
     vector<vector<cv::Point>> filtered_contours;
     find_contours(uProcessed, &filtered_contours, (int) this->_contourFiltMeth,
          this->umapParams.contour_filtering_thresh_min,
          this->umapParams.contour_filtering_thresh_max,
          &this->umapParams.contour_filtering_offset,
          &this->processingDebugger.filtered_umap_hierarchies,  // filtered contours visualization
          nullptr,  // all_contours visualization
          nullptr   // all_hierarchies visualization
     );
     this->processingDebugger.set_filtered_umap_contours(filtered_contours);
     // </custom-fold>

     if(this->_debug_process_timings){
          double tmpT = (double)cv::getTickCount();
          dt = (tmpT - t)/cv::getTickFrequency();
          printf("[DEBUG] %s::process_w_cuda() --- --- umap contour filtering took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
          dt = (tmpT - tUVmapProc)/cv::getTickFrequency();
          printf("[DEBUG] %s::process_w_cuda() --- --- Overall UV-map processing block took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
          t = (double)cv::getTickCount();
     }

     float delta_slope   = 0;
     int delta_intercept = 0;
     if(!line_params.empty()){
          float cur_gnd_line_slope       = (float) line_params[0];
          int cur_gnd_line_intercept     = (int) line_params[1];
          delta_slope                    = fabs(cur_gnd_line_slope - this->_prev_gnd_line_slope);
          delta_intercept                = abs(cur_gnd_line_intercept - this->_prev_gnd_line_intercept);
          this->_prev_gnd_line_slope     = cur_gnd_line_slope;
          this->_prev_gnd_line_intercept = cur_gnd_line_intercept;
          if(this->_check_gnd_line_noise){
               if(delta_slope > this->_delta_gnd_line_slope_thresh){
                    // Override ground line detection flag
                    gndPresent = false;
                    printf("[INFO] %s::process_w_cuda() --- Noisy Ground Line Detected, slope difference (%.3f) > %.3f.\r\n",
                         this->classLbl.c_str(), delta_slope, this->_delta_gnd_line_slope_thresh
                    );
               } else if(delta_intercept > this->_delta_gnd_line_intercept_thresh){
                    // Override ground line detection flag
                    gndPresent = false;
                    printf("[INFO] %s::process_w_cuda() --- Noisy Ground Line Detected, intercept difference (%.3f) > %.3f.\r\n",
                         this->classLbl.c_str(), delta_intercept, this->_delta_gnd_line_intercept_thresh
                    );
               }
          }
     }
     if(!gndPresent){
          printf("[INFO] %s::process_w_cuda() --- Unable to detect Ground Line.\r\n", this->classLbl.c_str());
          // Ensure any ground line dependent functions get notified on no ground line
          // via null coefficients
          line_params = std::vector<float>{};
          this->processingDebugger.set_gnd_line_coefficients(std::vector<float>{});
          if(line_coefficients) *line_coefficients = std::vector<float>{};
     } else{
          this->processingDebugger.set_gnd_line_coefficients(line_params);
          if(line_coefficients) *line_coefficients = std::vector<float>(line_params.begin(), line_params.end());
     }
     if(umap_output || this->processingDebugger.visualize_umap_final){
          this->processingDebugger.set_umap_processed(uProcessed);
          if(umap_output) *umap_output = uProcessed.clone();
     }
     if(vmap_output || this->processingDebugger.visualize_vmap_final){
          this->processingDebugger.set_vmap_processed(vProcessed);
          if(vmap_output) *vmap_output = vProcessed.clone();
     }
     // <custom-fold UV-Map Post-processing

     // Obstacle data extraction
     int nObs = 0;
     std::vector<Obstacle> obstacles_;
     std::vector< std::vector<cv::Rect> > obstacleRegions;
     if(this->_debug_process_timings) t = (double)cv::getTickCount();
     if(this->_do_obstacle_data_extraction){
          nObs = find_obstacles_disparity(vProcessed, filtered_contours, line_params, &obstacles_,
               &obstacleRegions    // obstacle regions visualization
          );

          if(this->_debug_process_timings){
               dt = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
               printf("[DEBUG] %s::process_w_cuda() --- --- finding individual obstacles took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
               t = (double)cv::getTickCount();
          }
     }
     this->processingDebugger.set_vmap_object_search_regions(obstacleRegions);

     // Filter the original depth image using all the useful data encoded within
     // the resulting processed UV-Maps
     if(this->_debug_process_timings) t = (double)cv::getTickCount();
     cv::Mat filtered_image;
     int err = filter_depth_using_ground_line(depthInput, disparityInput,
          vProcessed, line_params, &filtered_image,
          this->vmapParams.depth_filtering_gnd_line_intercept_offset,
          &this->processingDebugger,   // keep_mask visualization
          false
     );
     // int err = filter_depth_using_object_candidate_regions(depthInput, disparityInput,
     //      vProcessed, filtered_contours, &filtered_image, line_params,
     //      this->vmapParams.depth_filtering_gnd_line_intercept_offset, &this->processingDebugger
     // );
     // this->processingDebugger.set_gnd_line_intercept_offset(this->vmapParams.depth_filtering_gnd_line_intercept_offset);

     if(this->_debug_process_timings){
          dt = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
          printf("[DEBUG] %s::process_w_cuda() --- --- ground line based depth filtering took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
          t = (double)cv::getTickCount();
     }

     cv::Mat filtered_depth;
     err = filter_depth_using_object_candidate_regions(filtered_image, disparityInput,
          vProcessed, filtered_contours, &filtered_depth, line_params,
          this->vmapParams.depth_filtering_gnd_line_intercept_offset, &this->processingDebugger
     );
     this->processingDebugger.set_gnd_line_intercept_offset(this->vmapParams.depth_filtering_gnd_line_intercept_offset);
     // err = filter_depth_using_ground_line(filtered_image, disparityInput,
     //      vProcessed, line_params, &filtered_depth,
     //      this->vmapParams.depth_filtering_gnd_line_intercept_offset,
     //      &this->processingDebugger,   // keep_mask visualization
     //      false
     // );

     if(this->_debug_process_timings){
          dt = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
          printf("[DEBUG] %s::process_w_cuda() --- --- obstacle candidate based depth filtering took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
          t = (double)cv::getTickCount();
     }

     // Attempt to remove any noise (speckles) from the resulting filtered depth image
     cv::Mat final_depth;
     if(this->_denoise_filtered_depth){
          cv::Mat morphElement = cv::getStructuringElement(cv::MORPH_ELLIPSE,
               cv::Size(this->_filtered_depth_denoising_size, this->_filtered_depth_denoising_size)
          );
          cv::Mat morphedDepth;
          cv::morphologyEx(filtered_depth, morphedDepth, cv::MORPH_OPEN, morphElement);
          final_depth = morphedDepth.clone();
     } else final_depth = filtered_depth.clone();
     if(this->_do_angle_correction && this->_angle_correction_performed) final_depth = rotate_image(final_depth, -correctionAngle);

     if(this->_debug_process_timings){
          dt = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
          printf("[DEBUG] %s::process_w_cuda() --- --- final filtered depth corrections took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
          t = (double)cv::getTickCount();
     }

     std::vector<Obstacle> obstacles_output;
     if( (this->_do_obstacle_data_extraction) && (!obstacles_.empty()) ){
          for(Obstacle obj : obstacles_){
               obj.update(false, this->_baseline, this->_depth_scale,
                    std::vector<float>{this->_fx, this->_fy},
                    std::vector<float>{this->_px, this->_py},
                    this->_depth_deproject_gain, 1.0, verbose_obstacles
               );
               obstacles_output.push_back(obj);
          }
          if(this->_debug_process_timings){
               dt = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
               printf("[DEBUG] %s::process_w_cuda() --- --- individual obstacle geometric data calculations took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
          }
     } else{ obstacles_output.reserve(nObs); obstacles_output.assign(obstacles_.begin(), obstacles_.end()); }

     if(this->_debug_process_timings){
          dt = ((double)cv::getTickCount() - tUVmapProc)/cv::getTickFrequency();
          printf("[DEBUG] %s::process_w_cuda() --- --- Entire processing pipeline took %.4lf ms (%.2lf Hz).\r\n", this->classLbl.c_str(), dt*1000.0, (1.0/dt));
     }

     // Return Output images if requested before visualization
     if(found_obstacles) *found_obstacles = std::vector<Obstacle>(obstacles_output.begin(), obstacles_output.end());
     if(filtered_input) *filtered_input = final_depth.clone();
     return (int) obstacles_.size();
     // #else
     // return -999;
     // #endif
}

void Vboats::set_camera_info(cv::Mat K, float depth_scale, float baseline, bool verbose){
     if(this->_cam_info_count <= 10){
          this->_fx = (float)K.at<double>(0);
          this->_fy = (float)K.at<double>(4);
          this->_px = (float)K.at<double>(2);
          this->_py = (float)K.at<double>(5);
          this->_baseline = baseline;
          this->_depth_scale = depth_scale;
          this->_Tx = (float)(this->_fx * baseline);
          this->_depth2disparityFactor = this->_Tx / this->_depth_scale;

          if(verbose) printf("[INFO] %s::set_camera_info() ---- DepthScale = %.4f, Baseline = %.6f,  Focals = [%.2f, %.2f],  Principles = [%.2f, %.2f]\r\n", this->classLbl.c_str(), this->_depth_scale, this->_baseline, this->_fx, this->_fy, this->_px, this->_py);
          this->_have_cam_info = true;
     }
     this->_cam_info_count++;
}
void Vboats::set_camera_info(float fx, float fy, float px, float py, float depth_scale, float baseline, bool verbose){
     if(this->_cam_info_count <= 10){
          this->_fx = fx;
          this->_fy = fy;
          this->_px = px;
          this->_py = py;
          this->_baseline = baseline;
          this->_depth_scale = depth_scale;
          this->_Tx = (float)(this->_fx * baseline);
          this->_depth2disparityFactor = this->_Tx / this->_depth_scale;

          if(verbose) printf("[INFO] %s::set_camera_info() ---- DepthScale = %.4f, Baseline = %.6f,  Focals = [%.2f, %.2f],  Principles = [%.2f, %.2f]\r\n", this->classLbl.c_str(), this->_depth_scale, this->_baseline, this->_fx, this->_fy, this->_px, this->_py);
          this->_have_cam_info = true;
     }
     this->_cam_info_count++;
}
void Vboats::set_camera_orientation(double roll, double pitch, double yaw, bool verbose){
     this->_cam_roll  = roll;
     this->_cam_pitch = pitch;
     this->_cam_yaw   = yaw;

     if(verbose){
          printf("[INFO] %s::set_camera_orientation() --- "
               "Camera RPY = [%.4lf, %.4lf, %.4lf] [rad]\t\t (%.2lf, %.2lf, %.2lf) (deg)\r\n", this->classLbl.c_str(),
               roll, pitch, yaw, roll*M_RAD2DEG, pitch*M_RAD2DEG, yaw*M_RAD2DEG
          );
     }
}
void Vboats::set_camera_orientation(double x, double y, double z, double w, bool verbose){
     std::vector<double> euler = quaternion_to_euler(x, y, z, w);
     double roll, pitch, yaw;
     roll  = (double) euler[0];
     pitch = (double) euler[1];
     yaw   = (double) euler[2];

     this->_cam_roll  = roll;
     this->_cam_pitch = pitch;
     this->_cam_yaw   = yaw;

     if(verbose){
          printf("[INFO] %s::set_camera_orientation() --- "
               "Camera RPY = [%.4lf, %.4lf, %.4lf] [rad]\t\t (%.2lf, %.2lf, %.2lf) (deg)\r\n", this->classLbl.c_str(),
               roll, pitch, yaw, roll*M_RAD2DEG, pitch*M_RAD2DEG, yaw*M_RAD2DEG
          );
     }
}
void Vboats::set_camera_angle_offset(double offset_degrees){ this->_cam_angle_offset = offset_degrees * M_DEG2RAD; }
void Vboats::set_depth_denoising_kernel_size(int size){ this->_filtered_depth_denoising_size = size; }
void Vboats::set_absolute_minimum_depth(float value){ this->_hard_min_depth = value; }
void Vboats::set_absolute_maximum_depth(float value){ this->_hard_max_depth = value; }
void Vboats::set_object_dimension_limits_x(float min, float max){
     this->_object_min_dimension_x = min;
     this->_object_max_dimension_x = max;
}
void Vboats::set_object_dimension_limits_y(float min, float max){
     this->_object_min_dimension_y = min;
     this->_object_max_dimension_y = max;
}
void Vboats::flip_object_dimension_x_limits(bool flag){ this->_flip_object_dimension_x_limits = flag; }
void Vboats::flip_object_dimension_y_limits(bool flag){ this->_flip_object_dimension_y_limits = flag; }
void Vboats::set_contour_filtering_method(std::string method){
     if(strcmp(method.c_str(), "perimeter") == 0) this->_contourFiltMeth = PERIMETER_BASED;
     else if(strcmp(method.c_str(), "Perimeter") == 0) this->_contourFiltMeth = PERIMETER_BASED;
     else if(strcmp(method.c_str(), "area") == 0) this->_contourFiltMeth = AREA_BASED;
     else if(strcmp(method.c_str(), "Area") == 0) this->_contourFiltMeth = AREA_BASED;
     else this->_contourFiltMeth = PERIMETER_BASED;
}
void Vboats::set_umap_processing_method(std::string method){
     if(strcmp(method.c_str(), "strips") == 0) this->_umapFiltMeth = STRIPPING_METHOD;
     else if(strcmp(method.c_str(), "stripping") == 0) this->_umapFiltMeth = STRIPPING_METHOD;
     else if(strcmp(method.c_str(), "sobel") == 0) this->_umapFiltMeth = SOBELIZED_METHOD;
     else if(strcmp(method.c_str(), "sobelized") == 0) this->_umapFiltMeth = SOBELIZED_METHOD;
     else this->_umapFiltMeth = SOBELIZED_METHOD;
}
void Vboats::set_vmap_processing_method(std::string method){
     if(strcmp(method.c_str(), "strips") == 0) this->_vmapFiltMeth = STRIPPING_METHOD;
     else if(strcmp(method.c_str(), "stripping") == 0) this->_vmapFiltMeth = STRIPPING_METHOD;
     else if(strcmp(method.c_str(), "sobel") == 0) this->_vmapFiltMeth = SOBELIZED_METHOD;
     else if(strcmp(method.c_str(), "sobelized") == 0) this->_vmapFiltMeth = SOBELIZED_METHOD;
     else this->_vmapFiltMeth = SOBELIZED_METHOD;
}
void Vboats::set_image_angle_correction_type(std::string method){
     if(strcmp(method.c_str(), "roll") == 0) this->_angleCorrectionType = ROLL_CORRECTION;
     else if(strcmp(method.c_str(), "Roll") == 0) this->_angleCorrectionType = ROLL_CORRECTION;
     else if(strcmp(method.c_str(), "pitch") == 0) this->_angleCorrectionType = PITCH_CORRECTION;
     else if(strcmp(method.c_str(), "Pitch") == 0) this->_angleCorrectionType = PITCH_CORRECTION;
     else if(strcmp(method.c_str(), "yaw") == 0) this->_angleCorrectionType = YAW_CORRECTION;
     else if(strcmp(method.c_str(), "Yaw") == 0) this->_angleCorrectionType = YAW_CORRECTION;
     else this->_angleCorrectionType = ROLL_CORRECTION;
}
void Vboats::toggle_disparity_generation_debug_verbosity(bool flag){ this->_debug_disparity_gen = flag; }
void Vboats::set_gnd_line_slope_error_threshold(float value){ this->_delta_gnd_line_slope_thresh = value; }
void Vboats::set_gnd_line_intercept_error_threshold(int value){ this->_delta_gnd_line_intercept_thresh = value; }
void Vboats::enable_noisy_gnd_line_filtering(bool flag){ this->_check_gnd_line_noise = flag; }
void Vboats::enable_image_processing_timings_debug(bool flag){ this->_debug_process_timings = flag; }

bool Vboats::is_obstacle_data_extraction_performed(){ return this->_do_obstacle_data_extraction; }
bool Vboats::is_depth_denoising_performed(){ return this->_denoise_filtered_depth; }
bool Vboats::is_angle_correction_performed(){ return this->_do_angle_correction; }
double Vboats::get_depth_absolute_min(){ return (double) this->_hard_min_depth; }
double Vboats::get_depth_absolute_max(){ return (double) this->_hard_max_depth; }
double Vboats::get_correction_angle(bool in_degrees, bool flip_sign){
     double output;
     // Get the angle according to which axis we are correcting on
     if(this->_angleCorrectionType == ROLL_CORRECTION)           output = -this->_cam_roll - this->_cam_angle_offset;
     else if(this->_angleCorrectionType == PITCH_CORRECTION)     output = -this->_cam_pitch - this->_cam_angle_offset;
     else if(this->_angleCorrectionType == YAW_CORRECTION)       output = -this->_cam_yaw - this->_cam_angle_offset;
     else output = 0.0;
     // Convert to degrees if necessary
     if(in_degrees) output = output * M_RAD2DEG;
     if(flip_sign) output = -1.0 * output;
     return output;
}
ImageAngleCorrectionType Vboats::get_angle_correction_type(){ return this->_angleCorrectionType; }
std::vector<double> Vboats::get_camera_angles(){ return std::vector<double>{this->_cam_roll, this->_cam_pitch, this->_cam_yaw}; }

void Vboats::enable_angle_correction(bool flag){ this->_do_angle_correction = flag; }
void Vboats::enable_correction_angle_sign_flip(bool flag){ this->_flip_correction_angle_sign = flag; }
void Vboats::enable_filtered_depth_denoising(bool flag){ this->_denoise_filtered_depth = flag; }
void Vboats::enable_obstacle_data_extraction(bool flag){ this->_do_obstacle_data_extraction = flag; }
