#include <cstdio>

#include "FBX_Utility.h"
#include "FBX_MeshFuncs.h"

#include <fbxsdk.h>

enum class ExportArg
{
	NONE = 0x0,
	MESH = 0x1,
	ANIMATION = 0x2,
	SKELETON = 0x4,
	VICON = 0x8,
	SCALE = 0x10
};
template<>
struct EnableBitMaskOperators<ExportArg> { static const bool enable = true; };

ExportArg checkArgs(const char* arg)
{
	ExportArg bitflag = ExportArg::NONE;
	arg++; // skip '-'
	while (*arg) {
		switch (*arg) {
			case 'm':
				bitflag |= ExportArg::MESH;
				printf("Mesh out\n");
				break;
			case 'a':
				bitflag |= ExportArg::ANIMATION;
				printf("Animation out\n");
				break;
			case 's':
				bitflag |= ExportArg::SKELETON;
				printf("Skeleton out\n");
				break;
			case 'v':
				bitflag |= ExportArg::VICON;
				printf("Vicon format\n");
				break;
			default:
				break;
		}
		arg++;
	}
	return bitflag;
}

FbxNode* FindAttribute(FbxNode *_node, const FbxNodeAttribute::EType type);

FbxNode* FindAttributeParent(FbxNode *_node, const FbxNodeAttribute::EType type);

int main(const int argc, const char** argv)
{
	const char* help = "\nProvide fbx file and arguments for export: "
		"\n\t-m\tmesh"
		"\n\t-a\tanimation"
		"\n\t-s\tskeleton"
		"\n\t~<float>\tadjust export scale"
		"\n\t-v\tvicon\n";
	ExportArg export_flags = ExportArg::NONE;
	float scale_factor = 1.f;

	std::string fbx_to_read;
	if( argc < 3 ) {
		printf("%s", help);
		return 0;
	}
	else {
		for (int i = 1; i < argc; i++) {
			if (*argv[i] == '-') {					// parse args		
				export_flags |= checkArgs(argv[i]);
			}
			else if (*argv[i] == '~') {				// scale skeleton and animations
				string8 scale_arg = argv[i] + 1;
				if (!scale_arg.is_empty()) 
					scale_factor = strtof(scale_arg.str(), nullptr);
			}
			else {									// grab fbx file
				fbx_to_read = argv[i];
			}
		}
	}
	// create fbx manager object
	FbxManager* sdkManager = FbxManager::Create();

	// initialize settings
	FbxIOSettings *_IOSettings = FbxIOSettings::Create(sdkManager, IOSROOT);
	sdkManager->SetIOSettings(_IOSettings);

	// process input
	std::string fbx_path;
	std::string fbx_name;
	std::string fileProvided = fbx_to_read.c_str();
	size_t filePath = fileProvided.find_last_of("/\\");
	if (filePath != std::string::npos) {
		fbx_path = fileProvided.substr(0, filePath + 1).c_str();
	}

	size_t fileExtIndex = fileProvided.find_last_of('.');
	if (fileExtIndex == std::string::npos || fileProvided == "") {
		printf("Provide valid FBX file\n");
		exit(-1);
	}

	std::string fileExt = fileProvided.substr(fileExtIndex + 1);
	if (fileExt == "FBX" || fileExt == "fbx") {
		filePath = (filePath == std::string::npos) ? 0 : filePath + 1;
		fbx_name = fileProvided.substr(filePath, fileExtIndex - filePath);
		printf("Parsing %s\n\n", fbx_name.c_str());
	}
	else {
		printf("Invalid FBX file\n");
		exit(-1);
	}

	// Create importer
	FbxImporter* _importer = FbxImporter::Create(sdkManager, "");

	// initialize fbx object
	if (!_importer->Initialize(fileProvided.c_str(), -1,
							   sdkManager->GetIOSettings())) {
		printf("Call to FBX::Initialize() failed. \nError: %s\n\n",
			   _importer->GetStatus().GetErrorString());
		exit(-1);
	}

	// create scene for FBX file
	std::string sceneName = fileProvided.substr(0, fileExtIndex);
	FbxScene* fbx_scene = FbxScene::Create(sdkManager, sceneName.c_str());
	// destroy importer after importing scene
	_importer->Import(fbx_scene);
	_importer->Destroy();
	// get global time info
	FbxTime::EMode g_timemode = fbx_scene->GetGlobalSettings().GetTimeMode();
	const float fr_rate = FbxTime::GetFrameRate(g_timemode);
	printf("Scene frame rate: %.3f(s)", fr_rate);

	// Triangulate all meshes (buggy)
	FbxGeometryConverter lGeomConverter(sdkManager);
	lGeomConverter.Triangulate(fbx_scene, true);

	// recursively walk thru scene and get asset information
	FbxNode* rootNode = fbx_scene->GetRootNode();
	if (rootNode) {
		// create asset
		AssetFBX asset;
		asset.scale_factor = scale_factor;
		asset.m_fbxName.set(fbx_name.c_str());
		asset.m_fbxPath.set(fbx_path.c_str());
		if (bool(export_flags & ExportArg::VICON)) {
			asset.m_viconFormat = true;
		}
		printf("\n\n---------\nSkeleton\n---------\n\n");
		FbxNode *_node = FindAttribute(rootNode, fbxsdk::FbxNodeAttribute::eSkeleton);
		if (_node) {
			processSkeletonAsset(_node, 0, asset);
		}
		printf("\n\n---------\nAnimation\n---------\n\n");
		for (int i = 0; i < fbx_scene->GetSrcObjectCount<FbxAnimStack>(); i++) {
			FbxAnimStack* lAnimStack = fbx_scene->GetSrcObject<FbxAnimStack>(i);
			FbxString lOutputString = lAnimStack->GetName();
			printf("Animation Stack Name: %s\n", lOutputString.Buffer());

			processAnimation(rootNode, lAnimStack, asset, fr_rate,
							 lOutputString.Buffer());
			// export DDA (animations)
			if (bool(export_flags & ExportArg::ANIMATION)) {
				asset.exportAnimation();
			}
		}
		printf("\n\n----\nMesh\n----\n\n");
		FbxNode *mesh_parent_node = 
			FindAttributeParent(rootNode, fbxsdk::FbxNodeAttribute::eMesh);
		if (mesh_parent_node) {
			for (int i = 0; i < mesh_parent_node->GetChildCount(); i++) {
				FbxNode *_node = mesh_parent_node->GetChild(i);
				FbxNodeAttribute* attrib = _node->GetNodeAttribute();
				if (attrib) {
					FbxNodeAttribute::EType type = attrib->GetAttributeType();
					if (type == fbxsdk::FbxNodeAttribute::eMesh) {
						processAsset(_node, 
									asset, 
									bool(export_flags & ExportArg::SKELETON),
									bool(export_flags & ExportArg::MESH));
					}
				}
			}
		}
		// end of parsing
	}

	// destroy sdkManager when done
	sdkManager->Destroy();

	return 0;
}

FbxNode *FindAttribute(FbxNode * _node, const FbxNodeAttribute::EType type)
{
	for (int i = 0; i < _node->GetChildCount(); i++) {
		FbxNode *new_node = _node->GetChild(i);
		FbxNodeAttribute* attrib = new_node->GetNodeAttribute();
		if (attrib) {
			FbxNodeAttribute::EType new_type = attrib->GetAttributeType();
			if (new_type == type) {
				return new_node; // return current node
			}
		}
		new_node = FindAttribute(new_node, type);
		if (new_node) { return new_node; }
	}
	return nullptr;
}

FbxNode* FindAttributeParent(FbxNode *_node, const FbxNodeAttribute::EType type)
{
	for (int i = 0; i < _node->GetChildCount(); i++) {
		FbxNode *new_node = _node->GetChild(i);
		FbxNodeAttribute* attrib = new_node->GetNodeAttribute();
		if (attrib) {
			FbxNodeAttribute::EType new_type = attrib->GetAttributeType();
			if (new_type == type) {
				return _node; // return parent and not current node
			}
		}
		new_node = FindAttributeParent(new_node, type);
		if (new_node) { return new_node; }
	}
	return nullptr;
}