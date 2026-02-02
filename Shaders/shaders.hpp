#pragma once

/**
 * @file shaders.hpp
 * @brief Sources GLSL embarquées pour la démo (fond vidéo, lignes épaisses, faces pleines).
 *
 * @details
 * Tous les shaders ciblent OpenGL 3.3 Core (`#version 330 core`).
 * - BG_*   : rendu du fond vidéo (quad plein écran, texture 2D).
 * - LINE_* : rendu de lignes à épaisseur constante en pixels (via Geometry Shader).
 * - FACE_* : rendu de faces pleines (couleur uniforme, sans éclairage).
 *
 * @note Les chaînes sont null-terminées et peuvent être passées directement à glShaderSource().
 */

/// Vertex shader du fond vidéo : passe (aPos,aUV) -> (gl_Position,vUV).
extern const char* BG_VS;
/// Fragment shader du fond vidéo : échantillonne uTex aux UV.
extern const char* BG_FS;

/// Vertex shader de lignes : applique uMVP à aPos.
extern const char* LINE_VS;
/**
 * @brief Geometry shader de lignes épaisses (constantes en pixels).
 * @details
 * Entrée : `layout(lines)` ; sortie : `triangle_strip` (4 sommets).
 * - `uThicknessPx` : épaisseur en pixels
 * - `uViewport`    : (width,height) pour convertir pixels -> NDC
 * Construit un quad autour du segment projeté en NDC.
 */
extern const char* LINE_GS;
/// Fragment shader de lignes : sortie couleur uniforme `uColor`.
extern const char* LINE_FS;

/// Vertex shader des faces : applique uMVP à aPos.
extern const char* FACE_VS;
/// Fragment shader des faces : sortie couleur uniforme `uFaceColor`.
extern const char* FACE_FS;
