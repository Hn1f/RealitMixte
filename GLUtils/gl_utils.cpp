/**
 * @file gl_utils.cpp
 * @brief Implémentation des utilitaires OpenGL : compilation des shaders et linkage programme.
 */

 #include "gl_utils.hpp"
 #include <vector>
 #include <string>
 #include <iostream>
 #include <stdexcept>
 #include <algorithm>
 
 /**
  * @brief Compile un shader GLSL et retourne son handle.
  * @param type Type de shader (GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, GL_GEOMETRY_SHADER, etc.).
  * @param src Code source GLSL (chaîne C, non nulle).
  * @return GLuint Handle du shader compilé et valide.
  * @throws std::runtime_error Si la compilation échoue (log détaillé écrit sur stderr).
  */
 GLuint compileShader(GLenum type, const char* src){
     GLuint s = glCreateShader(type);
     glShaderSource(s, 1, &src, nullptr);
     glCompileShader(s);
 
     GLint ok = GL_FALSE;
     glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
     if (!ok) {
         GLint logLen = 0;
         glGetShaderiv(s, GL_INFO_LOG_LENGTH, &logLen);
         std::vector<GLchar> log(std::max(1, logLen));
         glGetShaderInfoLog(s, logLen, nullptr, log.data());
         std::cerr << "Shader compile error:\n" << log.data() << std::endl;
 
         glDeleteShader(s);
         throw std::runtime_error("Shader compile failed");
     }
     return s;
 }
 
 /**
  * @brief Lie un programme OpenGL à partir d'une liste de shaders compilés.
  * @param shaders Liste des shaders compilés à attacher (sera détruite après le link).
  * @return GLuint Handle du programme lié.
  * @throws std::runtime_error Si l’édition de liens échoue (log détaillé écrit sur stderr).
  *
  * @note Tous les shaders passés en argument sont supprimés via glDeleteShader()
  *       après l'appel à glLinkProgram(), succès ou échec, pour éviter les fuites.
  */
 GLuint linkProgram(const std::vector<GLuint>& shaders){
     GLuint p = glCreateProgram();
 
     for (auto s : shaders) glAttachShader(p, s);
     glLinkProgram(p);
 
     GLint ok = GL_FALSE;
     glGetProgramiv(p, GL_LINK_STATUS, &ok);
 
     // Quoi qu'il arrive, on libère les shaders (ils ne sont plus nécessaires après link)
     for (auto s : shaders) glDeleteShader(s);
 
     if (!ok) {
         GLint logLen = 0;
         glGetProgramiv(p, GL_INFO_LOG_LENGTH, &logLen);
         std::vector<GLchar> log(std::max(1, logLen));
         glGetProgramInfoLog(p, logLen, nullptr, log.data());
         std::cerr << "Program link error:\n" << log.data() << std::endl;
 
         glDeleteProgram(p);
         throw std::runtime_error("Program link failed");
     }
     return p;
 }
 