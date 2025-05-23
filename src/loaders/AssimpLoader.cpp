#include "AssimpLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace loaders;

void processMesh(aiMesh* mesh, const aiScene* scene, Model& model);
void processNode(aiNode* node, const aiScene* scene, Model& model) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, scene, model);
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, model);
    }
}


bool loaders::loadModel(const std::string& p, Model& model){
    Assimp::Importer importer;
    auto* scene = importer.ReadFile(p, aiProcess_Triangulate|aiProcess_FlipUVs|aiProcess_GenNormals|aiProcess_PreTransformVertices);
    if(!scene||scene->mFlags&AI_SCENE_FLAGS_INCOMPLETE||!scene->mRootNode) return false;
    processNode(scene->mRootNode, scene, model);
    return true;
}


void processMesh(aiMesh* mesh, const aiScene* scene, Model& model) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex v;

        v.pos = glm::vec3(
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z
                );

        if (mesh->HasNormals()) {
            v.norm = glm::vec3(
                    mesh->mNormals[i].x,
                    mesh->mNormals[i].y,
                    mesh->mNormals[i].z
                    );
        } else {
            v.norm = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        if (mesh->mTextureCoords[0]) {
            v.uv = glm::vec2(
                    mesh->mTextureCoords[0][i].x,
                    mesh->mTextureCoords[0][i].y
                    );
        } else {
            v.uv = glm::vec2(0.0f, 0.0f);
        }


        vertices.push_back(v);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    model.meshes.emplace_back(std::move(vertices), std::move(indices));
}
