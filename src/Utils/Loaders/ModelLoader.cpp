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
TextureInfo LoadMaterialTextures(aiMaterial* _material, aiTextureType _assimpType, MODEL_TEXTURE_TYPE _nekiType, const std::string& _directory);



Model ModelLoader::Load(const std::string& _filepath)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(_filepath,
	                                         aiProcess_Triangulate |		//Ensure model is composed of triangles
	                                         aiProcess_GenSmoothNormals |	//Generate smooth normals if they don't exist
	                                         aiProcess_FlipUVs |			//Flip UVs to match Vulkan's top left texcoord system
	                                         aiProcess_CalcTangentSpace		//Calculate tangents and bitangents (required for TBN in normal mapping)
	                                        );

	//Ensure scene was loaded correctly
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		throw std::runtime_error("Failed to load model (" + _filepath + ") - " + std::string(importer.GetErrorString()));
	}

	Model model;
	model.directory = _filepath.substr(0, _filepath.find_last_of('/'));
	ProcessNode(scene->mRootNode, scene, model);

	//Load scene materials
	model.materials.resize(scene->mNumMaterials);
	for (std::size_t i{ 0 }; i < scene->mNumMaterials; ++i)
	{
		aiMaterial* assimpMaterial{ scene->mMaterials[i] };
		Material nekiMaterial{};
		
		for (std::size_t textureType{ 0 }; textureType < static_cast<std::size_t>(MODEL_TEXTURE_TYPE::NUM_MODEL_TEXTURE_TYPES); ++textureType)
		{
			switch (static_cast<MODEL_TEXTURE_TYPE>(textureType))
			{
			case MODEL_TEXTURE_TYPE::DIFFUSE:
			{
				TextureInfo diffuseMaps{ LoadMaterialTextures(assimpMaterial, aiTextureType_DIFFUSE, MODEL_TEXTURE_TYPE::DIFFUSE, model.directory) };

				//Apparently, Assimp often loads diffuse maps as aiTextureType_BASE_COLOR, so load those in also if no textures were loaded in for aiTextureType_DIFFUSE
				if (diffuseMaps.paths.empty())
				{
					diffuseMaps = LoadMaterialTextures(assimpMaterial, aiTextureType_BASE_COLOR, MODEL_TEXTURE_TYPE::DIFFUSE, model.directory);
				}
				
				nekiMaterial.textures.push_back(diffuseMaps);
				
				break;
			}
			case MODEL_TEXTURE_TYPE::NORMAL:
			{
				TextureInfo normalMaps{ LoadMaterialTextures(assimpMaterial, aiTextureType_NORMALS, MODEL_TEXTURE_TYPE::NORMAL, model.directory) };

				//Apparently, Assimp often loads normal maps as aiTextureType_HEIGHT, so load those in also if no textures were loaded in for aiTextureType_NORMALS
				//Reference: https://github.com/JoeyDeVries/LearnOpenGL/issues/30
				if (normalMaps.paths.empty())
				{
					normalMaps = LoadMaterialTextures(assimpMaterial, aiTextureType_HEIGHT, MODEL_TEXTURE_TYPE::NORMAL, model.directory);
				}

				nekiMaterial.textures.push_back(normalMaps);
				
				break;
			}
			case MODEL_TEXTURE_TYPE::SPECULAR:
			{
				nekiMaterial.textures.push_back(LoadMaterialTextures(assimpMaterial, aiTextureType_SPECULAR, MODEL_TEXTURE_TYPE::SPECULAR, model.directory));
				break;
			}
			case MODEL_TEXTURE_TYPE::METALLIC:
			{
				nekiMaterial.textures.push_back(LoadMaterialTextures(assimpMaterial, aiTextureType_METALNESS, MODEL_TEXTURE_TYPE::METALLIC, model.directory));
				break;
			}
			case MODEL_TEXTURE_TYPE::ROUGHNESS:
			{
				nekiMaterial.textures.push_back(LoadMaterialTextures(assimpMaterial, aiTextureType_DIFFUSE_ROUGHNESS, MODEL_TEXTURE_TYPE::ROUGHNESS, model.directory));
				break;
			}
			case MODEL_TEXTURE_TYPE::AMBIENT_OCCLUSION:
			{
				//Apparently, Assimp often loads ao maps as aiTextureType_LIGHTMAP, so load those in also if no textures were loaded in for aiTextureType_AMBIENT_OCCLUSION
				TextureInfo aoMaps{ LoadMaterialTextures(assimpMaterial, aiTextureType_AMBIENT_OCCLUSION, MODEL_TEXTURE_TYPE::AMBIENT_OCCLUSION, model.directory) };
				if (aoMaps.paths.empty())
				{
					aoMaps = LoadMaterialTextures(assimpMaterial, aiTextureType_LIGHTMAP, MODEL_TEXTURE_TYPE::AMBIENT_OCCLUSION, model.directory);
				}

				nekiMaterial.textures.push_back(aoMaps);
				
				break;
			}
			case MODEL_TEXTURE_TYPE::EMISSIVE:
			{
				nekiMaterial.textures.push_back(LoadMaterialTextures(assimpMaterial, aiTextureType_EMISSIVE, MODEL_TEXTURE_TYPE::EMISSIVE, model.directory));
				break;
			}
			default:
				break;
			}
		}
		
		model.materials[i] = nekiMaterial;
	}

	return model;
}



void ModelLoader::ProcessNode(aiNode* _node, const aiScene* _scene, Model& _outModel)
{
	//Process all the node's meshes (if any)
	for (std::size_t i{ 0 }; i < _node->mNumMeshes; ++i)
	{
		aiMesh* mesh{ _scene->mMeshes[_node->mMeshes[i]] };
		_outModel.meshes.push_back(ProcessMesh(mesh, _scene, _outModel.directory));
	}

	//Recursively process each child node
	for (std::size_t i{ 0 }; i < _node->mNumChildren; ++i)
	{
		ProcessNode(_node->mChildren[i], _scene, _outModel);
	}
}



Mesh ModelLoader::ProcessMesh(aiMesh* _mesh, const aiScene* _scene, const std::string& _directory)
{
	Mesh nekiMesh;

	//Process vertices
	for (std::size_t i{ 0 }; i < _mesh->mNumVertices; ++i)
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
	for (std::size_t i{ 0 }; i < _mesh->mNumFaces; ++i)
	{
		//Append all indices on the face to our indices vector
		aiFace face{ _mesh->mFaces[i] };
		for (std::size_t j{ 0 }; j < face.mNumIndices; ++j)
		{
			nekiMesh.indices.push_back(face.mIndices[j]);
		}
	}


	nekiMesh.materialIndex = _mesh->mMaterialIndex;


	return nekiMesh;
}



TextureInfo LoadMaterialTextures(aiMaterial* _material, aiTextureType _assimpType, MODEL_TEXTURE_TYPE _nekiType, const std::string& _directory)
{
	//Loop through all textures of type _assimpType for _material and push path onto textureInfo's path list
	TextureInfo textureInfo{};
	textureInfo.type = _nekiType;
	for (std::size_t i{ 0 }; i < _material->GetTextureCount(_assimpType); ++i)
	{
		//Get file name of texture
		aiString filename;
		_material->GetTexture(_assimpType, i, &filename);
		textureInfo.paths.push_back(_directory + "/" + filename.C_Str());
	}
	return textureInfo;
}



}