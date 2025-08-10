#include "NekiVK/Utils/Loaders/ModelLoader.h"
#include <stdexcept>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Neki
{



//aiTextureType is an assimp enum with no underlying type
//->Enums without underlying types can't be forward declared in C++
//->Function can't be declared in ModelLoader.h
//->Take it out the class and forward declare in .cpp (this function is not user-accessible - it is for interna usage only)
//
//Load all texture paths of a given type from an Assimp material
std::vector<TextureInfo> LoadMaterialTextures(aiMaterial* _material, aiTextureType _assimpType, MODEL_TEXTURE_TYPE _nekiType, const std::string& _directory);


Model ModelLoader::Load(const std::string& _filepath)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(_filepath,
		aiProcess_Triangulate | //Ensure model is composed of triangles
		aiProcess_GenSmoothNormals | //Generate smooth normals if they don't exist
		aiProcess_FlipUVs | //Flip UVs to match Vulkan's top left texcoord system
		aiProcess_CalcTangentSpace //Calculate tangents and bitangents (required for TBN in normal mapping)
	);

	//Ensure scene was loaded correctly
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		throw std::runtime_error("Failed to load model (" + _filepath + ") - " + std::string(importer.GetErrorString()));
	}

	Model model;
	model.directory = _filepath.substr(0, _filepath.find_last_of('/'));
	ProcessNode(scene->mRootNode, scene, model);
	return model;
}



void ModelLoader::ProcessNode(aiNode* _node, const aiScene* _scene, Model& _outModel)
{
	//Process all the node's meshes (if any)
	for (std::size_t i{ 0 }; i<_node->mNumMeshes; ++i)
	{
		aiMesh* mesh{ _scene->mMeshes[_node->mMeshes[i]] };
		_outModel.meshes.push_back(ProcessMesh(mesh, _scene, _outModel.directory));
	}

	//Recursively process each child node
	for (std::size_t i{ 0 }; i<_node->mNumChildren; ++i)
	{
		ProcessNode(_node->mChildren[i], _scene, _outModel);
	}
}



Mesh ModelLoader::ProcessMesh(aiMesh* _mesh, const aiScene* _scene, const std::string& _directory)
{
	Mesh nekiMesh;

	//Process vertices
	for (std::size_t i{ 0 }; i<_mesh->mNumVertices; ++i)
	{
		ModelVertex vertex{};

		//Position
		vertex.position = { _mesh->mVertices[i].x, _mesh->mVertices[i].y, _mesh->mVertices[i].z };

		//Normal
		if (_mesh->HasNormals())
		{
			vertex.normal = { _mesh->mNormals[i].x, _mesh->mNormals[i].y, _mesh->mNormals[i].z };
		}

		//Assimp allows up to 8 texture coordinates per vertex for some reason. We only need the first set
		if (_mesh->mTextureCoords[0])
		{
			vertex.texCoord = { _mesh->mTextureCoords[0][i].x, _mesh->mTextureCoords[0][i].y };
		}

		//Tangent and Bitangent
		if (_mesh->HasTangentsAndBitangents())
		{
			vertex.tangent = { _mesh->mTangents[i].x, _mesh->mTangents[i].y, _mesh->mTangents[i].z };
			vertex.bitangent = { _mesh->mBitangents[i].x, _mesh->mBitangents[i].y, _mesh->mBitangents[i].z };
		}

		nekiMesh.vertices.push_back(vertex);
	}


	//Process indices
	for (std::size_t i{ 0 }; i<_mesh->mNumFaces; ++i)
	{
		//Append all indices on the face to our indices vector
		aiFace face{ _mesh->mFaces[i] };
		for (std::size_t j{ 0 }; j<face.mNumIndices; ++j)
		{
			nekiMesh.indices.push_back(face.mIndices[j]);
		}
	}


	//Process material (textures)
	aiMaterial* material{ _scene->mMaterials[_mesh->mMaterialIndex] };

	std::vector<TextureInfo> diffuseMaps{ LoadMaterialTextures(material, aiTextureType_DIFFUSE, MODEL_TEXTURE_TYPE::DIFFUSE, _directory) };
	nekiMesh.textures.insert(nekiMesh.textures.end(), diffuseMaps.begin(), diffuseMaps.end());

	std::vector<TextureInfo> normalMaps = LoadMaterialTextures(material, aiTextureType_NORMALS, MODEL_TEXTURE_TYPE::NORMAL, _directory);
	nekiMesh.textures.insert(nekiMesh.textures.end(), normalMaps.begin(), normalMaps.end());
	
	//Apparently, Assimp often loads normal maps as aiTextureType_HEIGHT, so load those in also if no textures were loaded in for aiTextureType_NORMALS
	//Reference: https://github.com/JoeyDeVries/LearnOpenGL/issues/30
	if (normalMaps.empty())
	{
		normalMaps = LoadMaterialTextures(material, aiTextureType_NORMALS, MODEL_TEXTURE_TYPE::NORMAL, _directory);
		nekiMesh.textures.insert(nekiMesh.textures.end(), normalMaps.begin(), normalMaps.end());
	}
	
	std::vector<TextureInfo> specularMaps = LoadMaterialTextures(material, aiTextureType_SPECULAR, MODEL_TEXTURE_TYPE::SPECULAR, _directory);
	nekiMesh.textures.insert(nekiMesh.textures.end(), specularMaps.begin(), specularMaps.end());

	std::vector<TextureInfo> metalnessMaps = LoadMaterialTextures(material, aiTextureType_METALNESS, MODEL_TEXTURE_TYPE::METALLIC, _directory);
	nekiMesh.textures.insert(nekiMesh.textures.end(), metalnessMaps.begin(), metalnessMaps.end());

	std::vector<TextureInfo> roughnessMaps = LoadMaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS, MODEL_TEXTURE_TYPE::ROUGHNESS, _directory);
	nekiMesh.textures.insert(nekiMesh.textures.end(), roughnessMaps.begin(), roughnessMaps.end());

	
	return nekiMesh;
}



std::vector<TextureInfo> LoadMaterialTextures(aiMaterial* _material, aiTextureType _assimpType, MODEL_TEXTURE_TYPE _nekiType, const std::string& _directory)
{
	//Loop through all textures of _assimpType for _material and push data onto textures vector
	std::vector<TextureInfo> textures;
	for (std::size_t i{ 0 }; i<_material->GetTextureCount(_assimpType); ++i)
	{
		//Get file name of texture
		aiString filename;
		_material->GetTexture(_assimpType, i, &filename);

		TextureInfo texture;
		texture.type = _nekiType;
		texture.path = _directory + '/' + std::string(filename.C_Str());

		textures.push_back(texture);
	}
	
	return textures;
}



}