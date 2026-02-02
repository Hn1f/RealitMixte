#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/aruco/charuco.hpp>

/**
 * @brief Estime la pose (rvec/tvec) d'une CharucoBoard dans une image.
 *
 * @param frameBGR   Image d'entrée (BGR/BGRA/GRAY)
 * @param K          Matrice intrinsèque 3x3 (CV_64F recommandé)
 * @param D          Coeffs distorsion (1xN ou Nx1, CV_64F recommandé)
 * @param board      CharucoBoard
 * @param rvec       (out) Rodrigues 3x1 (CV_64F)
 * @param tvec       (out) translation 3x1 (CV_64F)
 * @param outDebug   (opt) image debug (markers + charuco)
 *
 * @return true si pose estimée de façon fiable
 */
bool estimateCharucoPose(
    const cv::Mat& frameBGR,
    const cv::Mat& K, const cv::Mat& D,
    const cv::Ptr<cv::aruco::CharucoBoard>& board,
    cv::Mat& rvec, cv::Mat& tvec,
    cv::Mat* outDebug = nullptr
);
