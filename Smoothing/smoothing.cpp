// Smoothing/smoothing.cpp
#include "smoothing.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>

void PoseSmoother::smooth(cv::Mat& rvec, cv::Mat& tvec)
{
    if (rvec.empty() || tvec.empty()) return;

    // Force double pour stabilité numérique
    if (rvec.type() != CV_64F) rvec.convertTo(rvec, CV_64F);
    if (tvec.type() != CV_64F) tvec.convertTo(tvec, CV_64F);

    // Init
    if (!hasPose) {
        rPrev = rvec.clone();
        tPrev = tvec.clone();
        hasPose = true;
        return;
    }

    // ---------- Translation : EMA ----------
    tvec = (1.0 - alphaPose) * tPrev + alphaPose * tvec;
    tPrev = tvec.clone();

    // ---------- Rotation : rvec -> R -> quat -> SLERP -> R -> rvec ----------
    cv::Mat Rcur, Rprev;
    cv::Rodrigues(rvec, Rcur);
    cv::Rodrigues(rPrev, Rprev);

    auto toQuat = [&](const cv::Mat& R) {
        // OpenCV row-major -> GLM column-major
        glm::mat3 m(1.0f);
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                m[j][i] = static_cast<float>(R.at<double>(i, j));
        return glm::quat_cast(m);
    };

    glm::quat qCur = toQuat(Rcur);
    glm::quat qPre = toQuat(Rprev);

    // IMPORTANT : évite les sauts si le quaternion “change de signe”
    if (glm::dot(qCur, qPre) < 0.0f) qCur = -qCur;

    glm::quat qSm = glm::slerp(qPre, qCur, static_cast<float>(alphaPose));

    glm::mat3 mSm = glm::mat3_cast(qSm);
    cv::Mat Rsm(3, 3, CV_64F);
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            Rsm.at<double>(i, j) = static_cast<double>(mSm[j][i]); // GLM -> OpenCV

    cv::Rodrigues(Rsm, rvec);
    rPrev = rvec.clone();
}

void PtsSmoother::apply(std::vector<cv::Point2f>& pts)
{
    if (pts.size() != 4) return;

    if (!has) {
        for (int i = 0; i < 4; ++i) prev[i] = pts[i];
        has = true;
        return;
    }

    const float a = static_cast<float>(alpha);
    for (int i = 0; i < 4; ++i) {
        prev[i] = prev[i] * (1.0f - a) + pts[i] * a;
        pts[i]  = prev[i];
    }
}
