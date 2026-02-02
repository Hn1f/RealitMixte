#include "opencv_utils.hpp"

#include <vector>

bool estimateCharucoPose(
    const cv::Mat& frameBGR,
    const cv::Mat& K, const cv::Mat& D,
    const cv::Ptr<cv::aruco::CharucoBoard>& board,
    cv::Mat& rvec, cv::Mat& tvec,
    cv::Mat* outDebug
){
    if (frameBGR.empty() || K.empty() || board.empty()) return false;

    // --- Gray ---
    cv::Mat gray;
    if (frameBGR.channels() == 3)      cv::cvtColor(frameBGR, gray, cv::COLOR_BGR2GRAY);
    else if (frameBGR.channels() == 4) cv::cvtColor(frameBGR, gray, cv::COLOR_BGRA2GRAY);
    else                               gray = frameBGR;

    // --- Intrinsics / Distortion en double ---
    cv::Mat Kd = K;
    cv::Mat Dd = D;

    if (Kd.type() != CV_64F) Kd.convertTo(Kd, CV_64F);
    if (!Dd.empty() && Dd.type() != CV_64F) Dd.convertTo(Dd, CV_64F);
    if (!Dd.empty()) Dd = Dd.reshape(1, 1); // 1 x N

    // --- Dict (OpenCV 4.6 : champ public) ---
    // CharucoBoard n'a pas getDictionary() en 4.6, c'est board->dictionary
    cv::Ptr<cv::aruco::Dictionary> dict = board->dictionary;
    if (dict.empty()) return false;

    // --- Detect markers ---
    std::vector<int> markerIds;
    std::vector<std::vector<cv::Point2f>> markerCorners;

    // En 4.6, detectMarkers attend Ptr<DetectorParameters>
    cv::Ptr<cv::aruco::DetectorParameters> params = cv::aruco::DetectorParameters::create();
    params->cornerRefinementMethod       = cv::aruco::CORNER_REFINE_SUBPIX;
    params->cornerRefinementWinSize      = 5;
    params->cornerRefinementMaxIterations= 30;
    params->cornerRefinementMinAccuracy  = 0.01;

    cv::aruco::detectMarkers(gray, dict, markerCorners, markerIds, params);

    // Debug init
    cv::Mat dbg;
    if (outDebug) dbg = frameBGR.clone();

    if (markerIds.empty()) {
        if (outDebug) *outDebug = dbg;
        return false;
    }

    // Optionnel : utile si tu as des faux positifs / board partiellement visible
    // cv::aruco::refineDetectedMarkers(gray, board, markerCorners, markerIds, cv::noArray(), Kd, Dd);

    // --- Interpolate charuco corners ---
    cv::Mat charucoCorners, charucoIds;
    int nCorners = cv::aruco::interpolateCornersCharuco(
        markerCorners, markerIds, gray, board,
        charucoCorners, charucoIds, Kd, Dd
    );

    if (outDebug) {
        cv::aruco::drawDetectedMarkers(dbg, markerCorners, markerIds);
        if (nCorners > 0) cv::aruco::drawDetectedCornersCharuco(dbg, charucoCorners, charucoIds);
        *outDebug = dbg;
    }

    // Seuil minimal de stabilité (sinon ça "saute")
    if (nCorners < 8) return false;

    // --- Pose ---
    cv::Vec3d rv, tv;
    bool ok = cv::aruco::estimatePoseCharucoBoard(
        charucoCorners, charucoIds, board, Kd, Dd, rv, tv
    );
    if (!ok) return false;

    rvec = (cv::Mat_<double>(3,1) << rv[0], rv[1], rv[2]);
    tvec = (cv::Mat_<double>(3,1) << tv[0], tv[1], tv[2]);
    return true;
}
