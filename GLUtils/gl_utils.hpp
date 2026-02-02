#pragma once
#include <GL/glew.h>
#include <vector>

/**
 * @file gl_utils.hpp
 * @brief Utilitaires OpenGL pour compiler des shaders et lier des programmes.
 *
 * @details
 * Fournit deux helpers :
 *  - compileShader(type, src) : compile un shader et lève une exception si erreur.
 *  - linkProgram(shaders)     : lie un programme à partir d'une liste de shaders déjà compilés,
 *    détruit les shaders attachés après le link (succès ou échec), et lève en cas d'erreur.
 */

/**
 * @brief Compile un shader GLSL.
 * @param type Type de shader (GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, GL_GEOMETRY_SHADER, etc.).
 * @param src Pointeur vers le code source GLSL (chaîne C terminée par '\0').
 * @return GLuint Handle du shader compilé (à attacher ensuite à un programme).
 * @throws std::runtime_error si la compilation échoue (le log est écrit sur stderr).
 *
 * @warning Contexte OpenGL valide requis.
 * @note Le shader retourné doit être détruit (glDeleteShader) après avoir été attaché/lié,
 *       ou utilise linkProgram() qui s’occupe de le supprimer après le link.
 */
GLuint compileShader(GLenum type, const char* src);

/**
 * @brief Lie un programme OpenGL à partir d'une liste de shaders compilés.
 * @param shaders Vecteur des handles de shaders compilés (vertex, fragment, etc.).
 * @return GLuint Handle du programme lié (utilisable avec glUseProgram).
 * @throws std::runtime_error si l’édition de liens échoue (le log est écrit sur stderr).
 *
 * @details
 * - Attache tous les shaders, lance glLinkProgram.
 * - Détruit systématiquement les shaders attachés (glDeleteShader), même en cas d’échec.
 * - En cas d’échec, détruit aussi le programme (glDeleteProgram) puis lève.
 *
 * @warning Contexte OpenGL valide requis.
 */
GLuint linkProgram(const std::vector<GLuint>& shaders);
