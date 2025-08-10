#ifndef MODELLOADER_H
#define MODELLOADER_H

#include <string>
#include <vector>
#include <glm/glm.hpp>

//Forward declaration for Assimp types to avoid including Assimp headers in public NekiVK library
struct aiNode;
struct aiMesh;
struct aiScene;
struct aiMaterial;



namespace Neki
{



struct ModelVertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
	glm::vec3 tangent;
	glm::vec3 bitangent;
};

enum class MODEL_TEXTURE_TYPE
{
	DIFFUSE,
	NORMAL,
	SPECULAR,
	METALLIC,
	ROUGHNESS,
	AMBIENT_OCCLUSION,
};

struct TextureInfo
{
	MODEL_TEXTURE_TYPE type;
	std::string path;
};

//A single drawable entity. A model can be composed of multiple meshes
struct Mesh
{
	std::vector<ModelVertex> vertices;
	std::vector<std::uint32_t> indices;
	std::vector<TextureInfo> textures;
};

//Represents an entire model, containing all of its meshes and the directory it was loaded from (e.g.: Resource Files/A/B.obj -> directory = "Resource Files/A")
struct Model
{
	std::string directory;
	std::vector<Mesh> meshes;
};


class ModelLoader
{
public:
	//Loads a model from the specified file path
	//Throws std::runtime_error on failure
	static Model Load(const std::string& _filepath);

private:
	//Recursively process nodes in the Assimp scene graph
	static void ProcessNode(aiNode* _node, const aiScene* _scene, Model& _outModel);

	//Translate an Assimp mesh to a Neki::Mesh
	static Mesh ProcessMesh(aiMesh* _mesh, const aiScene* _scene, const std::string& _directory);
};



}



#endif