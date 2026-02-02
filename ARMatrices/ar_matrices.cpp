#include "ar_matrices.hpp"
#include <glm/gtc/type_ptr.hpp>

glm::mat4 projectionFromCV(const cv::Mat& K, float w, float h, float n, float f) {
    double fx = K.at<double>(0,0);
    double fy = K.at<double>(1,1);
    double cx = K.at<double>(0,2);
    double cy = K.at<double>(1,2);

    glm::mat4 P(0.0f);
    P[0][0] =  2.0f * static_cast<float>(fx) / w;
    P[1][1] =  2.0f * static_cast<float>(fy) / h;

    // centre optique (attention aux conventions)
    P[2][0] =  1.0f - 2.0f * static_cast<float>(cx) / w;
    P[2][1] =  2.0f * static_cast<float>(cy) / h - 1.0f;

    P[2][2] = -(f + n) / (f - n);
    P[2][3] = -1.0f;
    P[3][2] = -2.0f * f * n / (f - n);
    return P;
}

// Construit une matrice MODEL (board -> camera) puis convertit OpenCV -> OpenGL
glm::mat4 modelFromRvecTvec_OpenCVtoGL(const cv::Mat& rvec, const cv::Mat& tvec) {
    if (rvec.empty() || tvec.empty()) return glm::mat4(1.0f);

    cv::Mat R;
    cv::Rodrigues(rvec, R);

    // Matrice 4x4 OpenCV : [R|t]
    cv::Mat M = cv::Mat::eye(4, 4, CV_64F);
    R.copyTo(M(cv::Rect(0,0,3,3)));
    tvec.copyTo(M(cv::Rect(3,0,1,3)));

    // Conversion repère :
    // OpenCV: x→, y↓, z→(vers l'avant)
    // OpenGL: x→, y↑, z←(cam regarde -Z)
    cv::Mat cvToGl = (cv::Mat_<double>(4,4) <<
        1,  0,  0, 0,
        0, -1,  0, 0,
        0,  0, -1, 0,
        0,  0,  0, 1
    );

    cv::Mat Mgl = cvToGl * M;

    // OpenCV est row-major, GLM column-major => transpose
    cv::Mat M32f; Mgl.convertTo(M32f, CV_32F);
    cv::Mat MT = M32f.t();
    return glm::make_mat4(MT.ptr<float>());
}
