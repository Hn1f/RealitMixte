#pragma once
#include <GL/glew.h>
#include <vector>
#include <cstdint>
#include <random>
#include <stack>
#include <utility>
#include <algorithm>

/**
 * @file geometries.hpp
 * @brief Déclarations des géométries utilitaires OpenGL (quad plein écran, cubes, maze, sphere).
 *
 * @details
 * Fournit de petits helpers pour créer des maillages (VAO/VBO/EBO) prêts à dessiner :
 *  - Quad plein écran en NDC (fond vidéo texturé).
 *  - Cube filaire (GL_LINES).
 *  - Cube solide (GL_TRIANGLES).
 *  - Labyrinthe (lignes/solide).
 *  - Sphere (pour la balle).
 * La destruction est centralisée via destroyMesh().
 */

// Forward declaration : Maze est défini dans ball.h (ou un autre header)
class Maze;

/**
 * @struct Mesh
 * @brief Conteneur minimal d’un maillage OpenGL.
 */
struct Mesh { GLuint vao=0, vbo=0, ebo=0; GLsizei count=0; };

// ----- basics -----
Mesh createBackgroundQuad();
Mesh createCubeWireframeUnit(float size);
Mesh createCubeSolidUnit();
void destroyMesh(Mesh& m);

// ----- maze (tes fonctions existantes) -----
Mesh createMazeLines(float mazeW, float mazeH, float wallT, float wallH);

Mesh createMazeWallsSolid(
    float mazeW, float mazeH,
    int cellsX, int cellsY,
    float corridorW,   // largeur couloir (m)
    float wallT,       // épaisseur mur (m)
    float wallH        // hauteur mur (m)
);

// Variante (si tu l’utilises ailleurs) : walls 2D output
struct Wall2D {
    float x0, y0;   // min
    float x1, y1;   // max
};

// Si tu l’utilises vraiment dans ton code : tu peux l’implémenter plus tard.
// Ici je laisse juste la déclaration telle que tu l’avais.
Mesh createMazeWallsSolid(float sheetW, float sheetH,
    int cellsX, int cellsY,
    float corridorW, float wallT, float wallH,
    std::vector<Wall2D>& outWalls);

// ----- NEW : Maze mesh depuis TON objet Maze (rendu == collisions) -----
Mesh createMazeWallsSolidFromMaze(const Maze& maze, float wallH);

// ----- sphere (pour Ball::mesh = createSphere) -----
Mesh createSphere(float radius, int stacks, int slices);
