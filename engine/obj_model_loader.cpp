#include "obj_model_loader.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

AssetLoader::ObjModelLoader::ObjModelLoader()
{
	
}

AssetLoader::ObjModelLoader::~ObjModelLoader()
{

}

static void CalcNormal(float N[3], float v0[3], float v1[3], float v2[3]) {
	float v10[3];
	v10[0] = v1[0] - v0[0];
	v10[1] = v1[1] - v0[1];
	v10[2] = v1[2] - v0[2];

	float v20[3];
	v20[0] = v2[0] - v0[0];
	v20[1] = v2[1] - v0[1];
	v20[2] = v2[2] - v0[2];

	N[0] = v10[1] * v20[2] - v10[2] * v20[1];
	N[1] = v10[2] * v20[0] - v10[0] * v20[2];
	N[2] = v10[0] * v20[1] - v10[1] * v20[0];

	float len2 = N[0] * N[0] + N[1] * N[1] + N[2] * N[2];
	if (len2 > 0.0f) {
		float len = sqrtf(len2);

		N[0] /= len;
		N[1] /= len;
		N[2] /= len;
	}
}

std::optional<ECS::RenderComponent> AssetLoader::ObjModelLoader::LoadAssetFromFile(std::string InFileName)
{
	auto fileName = mModulePath.string() + "\\" + InFileName;
	if (!std::filesystem::exists(std::filesystem::path(fileName)))
	{
		return {};
	}
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	// Loop over shapes
	// shape = mesh
	ECS::RenderComponent newModelAsset;

		tinyobj::attrib_t attrib;
		auto mtlDir = std::filesystem::path(fileName).parent_path();
		bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials,&err, fileName.c_str(),(mtlDir.string() + "\\").c_str());
		if (!warn.empty()) {
			std::cout << "WARN: " << warn << std::endl;
		}
		if (!err.empty()) {
			std::cerr << err << std::endl;
		}

		spdlog::info("# of vertices  = %d\n", (int)(attrib.vertices.size()) / 3);
		spdlog::info("# of normals   = %d\n", (int)(attrib.normals.size()) / 3);
		spdlog::info("# of texcoords = %d\n", (int)(attrib.texcoords.size()) / 2);
		spdlog::info("# of materials = %d\n", (int)materials.size());
		spdlog::info("# of shapes    = %d\n", (int)shapes.size());

		// Append `default` material
		materials.push_back(tinyobj::material_t());

		for (size_t i = 0; i < materials.size(); i++) {
			printf("material[%d].diffuse_texname = %s\n", int(i),
				materials[i].diffuse_texname.c_str());
		}
		// Load diffuse textures
		{
			//for (size_t m = 0; m < materials.size(); m++) {
			//	tinyobj::material_t* mp = &materials[m];
			//
			//	if (mp->diffuse_texname.length() > 0) {
			//		// Only load the texture if it is not already loaded
			//		if (textures.find(mp->diffuse_texname) == textures.end()) {
			//			GLuint texture_id;
			//			int w, h;
			//			int comp;
			//
			//			std::string texture_filename = mp->diffuse_texname;
			//			if (!FileExists(texture_filename)) {
			//				// Append base dir.
			//				texture_filename = base_dir + mp->diffuse_texname;
			//				if (!FileExists(texture_filename)) {
			//					std::cerr << "Unable to find file: " << mp->diffuse_texname
			//						<< std::endl;
			//					exit(1);
			//				}
			//			}
			//
			//			unsigned char* image =
			//				stbi_load(texture_filename.c_str(), &w, &h, &comp, STBI_default);
			//			if (!image) {
			//				std::cerr << "Unable to load texture: " << texture_filename
			//					<< std::endl;
			//				exit(1);
			//			}
			//			std::cout << "Loaded texture: " << texture_filename << ", w = " << w
			//				<< ", h = " << h << ", comp = " << comp << std::endl;
			//
			//			glGenTextures(1, &texture_id);
			//			glBindTexture(GL_TEXTURE_2D, texture_id);
			//			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			//			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			//			if (comp == 3) {
			//				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB,
			//					GL_UNSIGNED_BYTE, image);
			//			}
			//			else if (comp == 4) {
			//				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
			//					GL_UNSIGNED_BYTE, image);
			//			}
			//			else {
			//				assert(0);  // TODO
			//			}
			//			glBindTexture(GL_TEXTURE_2D, 0);
			//			stbi_image_free(image);
			//			textures.insert(std::make_pair(mp->diffuse_texname, texture_id));
			//		}
			//	}
			//}
		}
		//shapes => one mesh
		newModelAsset.mMeshes.resize(1);
		auto verticesCount = attrib.vertices.size() / 3;
		auto& mesh = newModelAsset.mMeshes[0];
		mesh.mVertices.resize(verticesCount);
		for (size_t s = 0; s < shapes.size(); s++) 
		{
			
			for (size_t f = 0; f < shapes[s].mesh.indices.size(); f++)
			{
				int current_material_id = shapes[s].mesh.material_ids[f / 3];
				float diffuse[3];
				for (size_t i = 0; i < 3 && current_material_id > 0; i++) {
					diffuse[i] = materials[current_material_id].diffuse[i];
				}

				tinyobj::index_t idx = shapes[s].mesh.indices[f];
				mesh.mIndices.push_back(idx.vertex_index);
				auto& vertex = mesh.mVertices[idx.vertex_index];
				vertex.pos[0] = attrib.vertices[3 * idx.vertex_index + 0];
				vertex.pos[1] = attrib.vertices[3 * idx.vertex_index + 1];
				vertex.pos[2] = attrib.vertices[3 * idx.vertex_index + 2];
				vertex.pos[3] = 1.0f;
				vertex.normal[0] = attrib.normals[3 * idx.normal_index + 0];
				vertex.normal[1] = attrib.normals[3 * idx.normal_index + 1];
				vertex.normal[2] = attrib.normals[3 * idx.normal_index + 2];
				vertex.normal[3] = 1.0f;
				vertex.color[0] = diffuse[0];
				vertex.color[1] = diffuse[1];
				vertex.color[2] = diffuse[2];
				vertex.color[3] = 1.0f;


				if (idx.texcoord_index < 0)
				{
					vertex.textureCoord[0] = 0.0f;
					vertex.textureCoord[1] = 0.0f;
				}
				else
				{
					vertex.textureCoord[0] = attrib.texcoords[2 * idx.texcoord_index + 0];
					vertex.textureCoord[1] = attrib.texcoords[2 * idx.texcoord_index + 1];
				}
			}
		}
	return newModelAsset;
}



