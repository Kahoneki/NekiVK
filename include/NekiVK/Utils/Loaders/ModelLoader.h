#ifndef MODELLOADER_H
#define MODELLOADER_H

#include <vulkan/vulkan.h>
#include <string>
#include <unordered_map>
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

enum class MODEL_TEXTURE_TYPE : std::uint32_t
{
	DIFFUSE           = 0,
	NORMAL            = 1,
	SPECULAR          = 2,
	METALLIC          = 3,
	ROUGHNESS         = 4,
	AMBIENT_OCCLUSION = 5,
	EMISSIVE          = 6,

	NUM_MODEL_TEXTURE_TYPES = 7,
};

//Contains all the textures for a specific MODEL_TEXTURE_TYPE
struct TextureInfo
{
	MODEL_TEXTURE_TYPE type;
	std::vector<std::string> paths; //Paths to all the textures of this type
};

struct Material
{
	std::vector<TextureInfo> textures;
};

//A single drawable entity. A model can be composed of multiple meshes
struct Mesh
{
	std::vector<ModelVertex> vertices;
	std::vector<std::uint32_t> indices;
	std::size_t materialIndex;
};

//Represents an entire model, containing all of its meshes and the directory it was loaded from (e.g.: Resource Files/A/B.obj -> directory = "Resource Files/A")
struct Model
{
	std::string directory;
	std::vector<Mesh> meshes;
	std::vector<Material> materials;
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