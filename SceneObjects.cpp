
/**
 * @file SceneObjects.cpp
 * @brief Implémentation des fonctions de chargement et de gestion des maillages OBJ.
 * @author Hanif Laarabi
 * @date 26/01/2026
 */

#include "SceneObjects.hpp"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <iostream>

 
 /**
  * @struct VKey
  * @brief Clé personnalisée pour identifier un sommet dans un fichier OBJ.
  * @details Utilisée pour dédupliquer les sommets en tenant compte des indices de position, texture et normale.
  */
 struct VKey {
     int vi; ///< Indice de position (vertex index).
     int ti; ///< Indice de texture (texture index).
     int ni; ///< Indice de normale (normal index).
 
     /**
      * @brief Opérateur de comparaison pour VKey.
      * @param o Autre clé à comparer.
      * @return true si les clés sont identiques, false sinon.
      */
     bool operator==(const VKey& o) const {
         return vi == o.vi && ti == o.ti && ni == o.ni;
     }
 };
 
 /**
  * @struct VKeyHash
  * @brief Fonction de hachage pour VKey, utilisée dans std::unordered_map.
  */
 struct VKeyHash {
     /**
      * @brief Calcule le hash d'une clé VKey.
      * @param k Clé à hacher.
      * @return size_t Valeur de hachage.
      */
     size_t operator()(const VKey& k) const noexcept {
         return (size_t)(k.vi * 73856093) ^ (size_t)(k.ti * 19349663) ^ (size_t)(k.ni * 83492791);
     }
 };
 
 /**
  * @brief Parse un triplet d'indices OBJ (ex: "1/2/3" ou "1//3").
  * @param token Chaîne à parser (ex: "1/2/3").
  * @param vi Indice de position (output).
  * @param ti Indice de texture (output).
  * @param ni Indice de normale (output).
  * @return true si le parsing a réussi, false sinon.
  */
 static bool parseTriplet(const std::string& token, int& vi, int& ti, int& ni) {
     vi = ti = ni = 0;
     int slashCount = 0;
     for (char c : token) if (c == '/') slashCount++;
 
     if (slashCount == 0) {
         vi = std::stoi(token);
         return true;
     }
 
     if (slashCount == 1) {
         size_t p = token.find('/');
         auto s0 = token.substr(0, p);
         auto s1 = token.substr(p + 1);
         vi = s0.empty() ? 0 : std::stoi(s0);
         ti = s1.empty() ? 0 : std::stoi(s1);
         return true;
     }
 
     size_t p1 = token.find('/');
     size_t p2 = token.find('/', p1 + 1);
     auto s0 = token.substr(0, p1);
     auto s1 = token.substr(p1 + 1, p2 - (p1 + 1));
     auto s2 = token.substr(p2 + 1);
     vi = s0.empty() ? 0 : std::stoi(s0);
     if (!s1.empty()) ti = std::stoi(s1);
     if (!s2.empty()) ni = std::stoi(s2);
     return true;
 }
 
 /**
  * @brief Corrige un indice OBJ pour le convertir en indice 0-based.
  * @param idx Indice à corriger.
  * @param n Taille du tableau de référence.
  * @return int Indice corrigé.
  */
 static int fixIndex(int idx, int n) {
     if (idx > 0) return idx - 1;
     if (idx < 0) return n + idx;
     return -1;
 }
 
 /**
  * @brief Charge les données de position et d'indices dans un VAO OpenGL.
  * @param pos Tableau de positions (float[3] par sommet).
  * @param idx Tableau d'indices (uint32_t).
  * @return Mesh Maillage prêt à être rendu.
  */
 static Mesh upload_PosOnly(const std::vector<float>& pos, const std::vector<uint32_t>& idx) {
     Mesh m{};
     m.count = (GLsizei)idx.size();
 
     glGenVertexArrays(1, &m.vao);
     glGenBuffers(1, &m.vbo);
     glGenBuffers(1, &m.ebo);
 
     glBindVertexArray(m.vao);
 
     glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
     glBufferData(GL_ARRAY_BUFFER, pos.size() * sizeof(float), pos.data(), GL_STATIC_DRAW);
 
     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
     glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(uint32_t), idx.data(), GL_STATIC_DRAW);
 
     glEnableVertexAttribArray(0);
     glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
 
     glBindVertexArray(0);
     return m;
 }
 
 /**
  * @brief Charge un maillage depuis un fichier OBJ.
  * @param path Chemin vers le fichier OBJ.
  * @return Mesh Maillage chargé (ou vide en cas d'erreur).
  */
 Mesh loadOBJMesh(const std::string& path) {
     std::ifstream f(path);
     if (!f.is_open()) {
         std::cerr << "[OBJ] Cannot open: " << path << "\n";
         return Mesh{};
     }
 
     std::vector<glm::vec3> V;
     V.reserve(10000);
 
     std::vector<float> outPos;
     outPos.reserve(30000);
     std::vector<uint32_t> outIdx;
     outIdx.reserve(60000);
 
     std::unordered_map<VKey, uint32_t, VKeyHash> dedup;
     dedup.reserve(60000);
 
     std::string line;
     while (std::getline(f, line)) {
         if (line.empty() || line[0] == '#') continue;
 
         std::istringstream iss(line);
         std::string tag;
         iss >> tag;
 
         if (tag == "v") {
             glm::vec3 p;
             iss >> p.x >> p.y >> p.z;
             V.push_back(p);
         }
         else if (tag == "f") {
             std::vector<VKey> face;
             std::string tok;
             while (iss >> tok) {
                 int vi, ti, ni;
                 if (!parseTriplet(tok, vi, ti, ni)) continue;
 
                 VKey k;
                 k.vi = fixIndex(vi, (int)V.size());
                 k.ti = (ti == 0) ? -1 : 0;
                 k.ni = (ni == 0) ? -1 : 0;
 
                 if (k.vi < 0 || k.vi >= (int)V.size()) continue;
                 face.push_back(k);
             }
 
             if (face.size() < 3) continue;
 
             auto getOrCreate = [&](const VKey& k) -> uint32_t {
                 auto it = dedup.find(k);
                 if (it != dedup.end()) return it->second;
 
                 const glm::vec3& p = V[k.vi];
                 uint32_t id = (uint32_t)(outPos.size() / 3);
                 outPos.push_back(p.x);
                 outPos.push_back(p.y);
                 outPos.push_back(p.z);
 
                 dedup.emplace(k, id);
                 return id;
             };
 
             uint32_t i0 = getOrCreate(face[0]);
             for (size_t i = 1; i + 1 < face.size(); ++i) {
                 uint32_t i1 = getOrCreate(face[i]);
                 uint32_t i2 = getOrCreate(face[i + 1]);
                 outIdx.push_back(i0);
                 outIdx.push_back(i1);
                 outIdx.push_back(i2);
             }
         }
     }
 
     if (outIdx.empty() || outPos.empty()) {
         std::cerr << "[OBJ] No geometry parsed from: " << path << "\n";
         return Mesh{};
     }
 
     return upload_PosOnly(outPos, outIdx);
 }
 
 /**
 * @brief Destructeur de SceneObjects.
 * Libère les ressources OpenGL des maillages.
 */
SceneObjects::~SceneObjects() {
    destroy();
}

/**
 * @brief Ajoute un objet OBJ à la scène.
 * @param path Chemin vers le fichier OBJ.
 * @param pos Position de l'objet.
 * @param rotDeg Rotation de l'objet (en degrés).
 * @param scale Échelle de l'objet.
 * @param color Couleur de l'objet (par défaut : gris clair).
 * @return Index de l'objet ajouté.
 */
int SceneObjects::addOBJ(
    const std::string& path,
    glm::vec3 pos,
    glm::vec3 rotDeg,
    glm::vec3 scale,
    glm::vec4 color)
{
    Item it;
    it.path = path;
    it.mesh = loadOBJMesh(path);
    it.pos = pos;
    it.rotDeg = rotDeg;
    it.scale = scale;
    it.color = color;
    items.push_back(it);
    return (int)items.size() - 1;
}

/**
 * @brief Dessine tous les objets de la scène.
 * @param progFace Programme OpenGL pour le rendu.
 * @param uMVP Localisation de l'uniforme MVP dans le shader.
 * @param uColor Localisation de l'uniforme de couleur dans le shader.
 * @param MVP_maze Matrice MVP globale.
 */
void SceneObjects::drawAll(
    GLuint progFace,
    GLint uMVP,
    GLint uColor,
    const glm::mat4& MVP_maze) const
{
    glUseProgram(progFace);
    for (const auto& it : items) {
        if (!it.visible || it.mesh.vao == 0 || it.mesh.count == 0) continue;

        glm::mat4 M(1.0f);
        M = glm::translate(M, it.pos);
        M = glm::rotate(M, glm::radians(it.rotDeg.x), glm::vec3(1,0,0));
        M = glm::rotate(M, glm::radians(it.rotDeg.y), glm::vec3(0,1,0));
        M = glm::rotate(M, glm::radians(it.rotDeg.z), glm::vec3(0,0,1));
        M = glm::scale(M, it.scale);

        glm::mat4 MVP = MVP_maze * M;
        glUniformMatrix4fv(uMVP, 1, GL_FALSE, &MVP[0][0]);
        glUniform4f(uColor, it.color.r, it.color.g, it.color.b, it.color.a);

        glBindVertexArray(it.mesh.vao);
        glDrawElements(GL_TRIANGLES, it.mesh.count, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}

/**
 * @brief Libère les ressources OpenGL de tous les objets.
 */
void SceneObjects::destroy() {
    for (auto& it : items) {
        if (it.mesh.vao != 0) glDeleteVertexArrays(1, &it.mesh.vao);
        if (it.mesh.vbo != 0) glDeleteBuffers(1, &it.mesh.vbo);
        if (it.mesh.ebo != 0) glDeleteBuffers(1, &it.mesh.ebo);
    }
    items.clear();
}
