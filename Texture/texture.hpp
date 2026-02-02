#pragma once
#include <GL/glew.h>
#include <string>
#include <opencv2/opencv.hpp>

/**
 * @file texture.hpp
 * @brief Utilitaires de texture OpenGL pour uploader des images OpenCV.
 *
 * @details
 * Fournit un helper pour créer/mettre à jour une texture 2D OpenGL à partir d'un `cv::Mat`
 * en gérant la conversion d'espace couleur (BGR/BGRA/GRAY -> RGB/RGBA) côté CPU.
 */

/**
 * @brief Crée ou met à jour une texture 2D OpenGL avec une image OpenCV.
 *
 * @param tex Handle existant de texture (0 pour en créer une nouvelle via glGenTextures).
 * @param imgBGR Image OpenCV source (BGR, BGRA ou GRAY).
 * @return GLuint Handle de la texture OpenGL valide (créée ou réutilisée).
 *
 * @details
 * - Convertit l'image en RGB (ou RGBA si entrée BGRA) avant upload.
 * - Alloue/écrit avec `glTexImage2D` (niveau 0), format interne `GL_RGB8`.
 * - Min/Mag filter = `GL_LINEAR`, wrap = `GL_CLAMP_TO_EDGE`.
 *
 * @warning Nécessite un contexte OpenGL actif.
 * @note Si l’entrée a 4 canaux, tu peux passer le format interne à `GL_RGBA8`
 *       et le format source à `GL_RGBA` (voir variante commentée).
 */
GLuint createOrUpdateTexture(GLuint tex, const cv::Mat& imgBGR);


// ✅ nouveau : charge un JPG/PNG depuis disque et crée une texture GL
GLuint loadTextureFromFile(const std::string& path, bool flipY = true);
