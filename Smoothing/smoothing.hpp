#pragma once
#include <opencv2/opencv.hpp>
#include <array>
#include <vector>

/**
 * @file smoothing.hpp
 * @brief Lissages simples pour la pose (rvec/tvec) et pour 4 points image.
 *
 * @details
 * Implémente un lissage exponentiel (EMA) :
 *   new = alpha * current + (1 - alpha) * previous
 * - PoseSmoother : sur (rvec, tvec) issus d’un solvePnP.
 * - PtsSmoother  : sur 4 points 2D ordonnés (TL, TR, BR, BL).
 *
 * @note Les tampons internes sont initialisés au premier appel (pas de prérequis).
 */

/**
 * @struct PoseSmoother
 * @brief Lissage exponentiel des vecteurs de pose OpenCV (rvec, tvec).
 *
 * @var PoseSmoother::hasPose
 *  Indique si une pose précédente a déjà été mémorisée.
 * @var PoseSmoother::rPrev
 *  Dernier rvec (Rodrigues, 3x1, CV_64F) mémorisé.
 * @var PoseSmoother::tPrev
 *  Dernier tvec (3x1, CV_64F) mémorisé.
 * @var PoseSmoother::alphaPose
 *  Coefficient EMA dans [0,1] (1 = pas de lissage, 0 = figé sur l’historique).
 */
struct PoseSmoother {
    bool hasPose = false;      ///< Pose précédente disponible ?
    cv::Mat rPrev, tPrev;      ///< Buffers rvec/tvec précédents (CV_64F attendus).
    double alphaPose = 0.20;   ///< Poids du courant dans l’EMA.

    /**
     * @brief Applique un lissage EMA sur (rvec, tvec) en place.
     * @param rvec Vecteur de rotation Rodrigues (3x1). Modifié par lissage.
     * @param tvec Vecteur de translation (3x1). Modifié par lissage.
     *
     * @details
     * Si aucune pose précédente n’est disponible, initialise l’historique avec (rvec, tvec).
     * Sinon, effectue : r = a*r + (1-a)*rPrev ; t = a*t + (1-a)*tPrev ; puis met à jour l’historique.
     * Les matrices sont converties si nécessaire en CV_64F pour le calcul.
     */
    void smooth(cv::Mat& rvec, cv::Mat& tvec);
};

/**
 * @struct PtsSmoother
 * @brief Lissage exponentiel d’un quadruplet de points image 2D (ordre TL, TR, BR, BL).
 *
 * @var PtsSmoother::has
 *  Indique si un quadruplet précédent existe.
 * @var PtsSmoother::prev
 *  Quadruplet précédent mémorisé.
 * @var PtsSmoother::alpha
 *  Coefficient EMA dans [0,1] (1 = pas de lissage).
 */
struct PtsSmoother {
    bool has = false;                                   ///< Points précédents disponibles ?
    std::array<cv::Point2f,4> prev{};                   ///< Historique (TL, TR, BR, BL).
    double alpha = 0.35;                                ///< Poids du courant dans l’EMA.

    /**
     * @brief Lisse en place un vecteur de 4 points (TL, TR, BR, BL).
     * @param pts Vecteur de 4 points ordonnés. Modifié par lissage.
     *
     * @details
     * - Au premier appel, mémorise simplement `pts` comme historique.
     * - Par la suite, pour i=0..3 : pts[i] = a*pts[i] + (1-a)*prev[i].
     * - Met à jour `prev` avec la valeur lissée.
     * @warning `pts.size()` doit être >= 4 et respecter l’ordre attendu.
     */
    void apply(std::vector<cv::Point2f>& pts);
};
