#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Geometries/geometries.hpp"

/**
 * @brief Charge un maillage OBJ depuis un fichier.
 * @param path Chemin vers le fichier OBJ.
 * @return Mesh Structure contenant les données du maillage.
 */
Mesh loadOBJMesh(const std::string& path);

/**
 * @class SceneObjects
 * @brief Classe pour gérer une collection d'objets 3D dans une scène.
 */
class SceneObjects
{
public:
    /**
     * @struct Item
     * @brief Structure représentant un objet 3D dans la scène.
     */
    struct Item
    {
        std::string path;     ///< Chemin vers le fichier OBJ.
        Mesh mesh{};          ///< Maillage de l'objet.
        glm::vec3 pos{0,0,0};        ///< Position de l'objet.
        glm::vec3 rotDeg{0,0,0};     ///< Rotation de l'objet en degrés (Euler angles).
        glm::vec3 scale{1,1,1};      ///< Échelle de l'objet.
        glm::vec4 color{0.8f,0.8f,0.8f,1.0f}; ///< Couleur de l'objet (RGBA).
        bool visible = true;  ///< Visibilité de l'objet.
    };

    /**
     * @brief Destructeur de la classe SceneObjects.
     */
    ~SceneObjects();

    /**
     * @brief Ajoute un objet 3D à la scène à partir d'un fichier OBJ.
     * @param path Chemin vers le fichier OBJ.
     * @param pos Position de l'objet.
     * @param rotDeg Rotation de l'objet en degrés.
     * @param scale Échelle de l'objet.
     * @param color Couleur de l'objet (par défaut : gris clair).
     * @return int Index de l'objet ajouté.
     */
    int addOBJ(
        const std::string& path,
        glm::vec3 pos,
        glm::vec3 rotDeg,
        glm::vec3 scale,
        glm::vec4 color = glm::vec4(0.8f,0.8f,0.8f,1.0f)
    );

    /**
     * @brief Dessine tous les objets visibles de la scène.
     * @param progFace Identifiant du programme de shader.
     * @param uMVP Localisation de l'uniforme MVP dans le shader.
     * @param uColor Localisation de l'uniforme de couleur dans le shader.
     * @param MVP_maze Matrice MVP de la scène.
     */
    void drawAll(
        GLuint progFace,
        GLint uMVP,
        GLint uColor,
        const glm::mat4& MVP_maze
    ) const;

    /**
     * @brief Libère les ressources OpenGL de tous les objets de la scène.
     */
    void destroy();

private:
    std::vector<Item> items; ///< Liste des objets de la scène.
};
