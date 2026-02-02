#pragma once
#include <opencv2/opencv.hpp>
#include <glm/glm.hpp>

// Projection OpenGL à partir des intrinsèques OpenCV
glm::mat4 projectionFromCV(const cv::Mat& K, float w, float h, float n, float f);

// Matrice Model OpenGL (board -> caméra), convertie dans le repère OpenGL
glm::mat4 modelFromRvecTvec_OpenCVtoGL(const cv::Mat& rvec, const cv::Mat& tvec);
