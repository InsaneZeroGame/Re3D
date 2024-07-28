#include "fbx_loader.h"
#include <execution>
#include <algorithm>
#ifdef IOS_REF
#undef IOS_REF
#define IOS_REF (*(pManager->GetIOSettings()))
#endif
#define MAT_HEADER_LENGTH 200

bool gVerbose = true;
// Four floats for every position.
const int POSITION_STRIDE = 4;
// Three floats for every normal.
const int NORMAL_STRIDE = 3;
// Two floats for every UV.
const int UV_STRIDE = 2;
const int TRIANGLE_VERTEX_COUNT = 3;


static void PrintString(FbxString& pString) {
    bool lReplaced = pString.ReplaceAll("%", "%%");
    FBX_ASSERT(lReplaced == false);
    FBXSDK_printf(pString);
}

void DisplayMetaDataConnections(FbxObject* pNode);
void DisplayString(const char* pHeader, const char* pValue = "", const char* pSuffix = "");
void DisplayBool(const char* pHeader, bool pValue, const char* pSuffix = "");
void DisplayInt(const char* pHeader, int pValue, const char* pSuffix = "");
void DisplayDouble(const char* pHeader, double pValue, const char* pSuffix = "");
void Display2DVector(const char* pHeader, FbxVector2 pValue, const char* pSuffix = "");
void Display3DVector(const char* pHeader, FbxVector4 pValue, const char* pSuffix = "");
void DisplayColor(const char* pHeader, FbxColor pValue, const char* pSuffix = "");
void Display4DVector(const char* pHeader, FbxVector4 pValue, const char* pSuffix = "");
void DisplayControlsPoints(FbxMesh* pMesh);
void DisplayMaterialMapping(FbxMesh* pMesh);
void DisplayTextureMapping(FbxMesh* pMesh);
void DisplayTextureNames(FbxProperty& pProperty, FbxString& pConnectionString);
void DisplayMaterialConnections(const FbxMesh* pMesh,ECS::StaticMesh& InMesh);
void DisplayMaterialTextureConnections(FbxSurfaceMaterial* pMaterial, ECS::StaticMesh& InMesh, char* header, int pMatId, int l);
static const FbxImplementation* LookForImplementation(FbxSurfaceMaterial* pMaterial) {
    const FbxImplementation* lImplementation = nullptr;
    if (!lImplementation)
        lImplementation = GetImplementation(pMaterial, FBXSDK_IMPLEMENTATION_CGFX);
    if (!lImplementation)
        lImplementation = GetImplementation(pMaterial, FBXSDK_IMPLEMENTATION_HLSL);
    if (!lImplementation)
        lImplementation = GetImplementation(pMaterial, FBXSDK_IMPLEMENTATION_SFX);
    if (!lImplementation)
        lImplementation = GetImplementation(pMaterial, FBXSDK_IMPLEMENTATION_OGS);
    if (!lImplementation)
        lImplementation = GetImplementation(pMaterial, FBXSDK_IMPLEMENTATION_SSSL);
    return lImplementation;
}
static void InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene) {
    //The first thing to do is to create the FBX Manager which is the object allocator for almost all the classes in the SDK
    pManager = FbxManager::Create();
    if (!pManager) {
        FBXSDK_printf("Error: Unable to create FBX Manager!\n");
        exit(1);
    } else
        FBXSDK_printf("Autodesk FBX SDK version %s\n", pManager->GetVersion());

    //Create an IOSettings object. This object holds all import/export settings.
    FbxIOSettings* ios = FbxIOSettings::Create(pManager, IOSROOT);
    pManager->SetIOSettings(ios);

#ifndef FBXSDK_ENV_WINSTORE
    //Load plugins from the executable directory (optional)
    FbxString lPath = FbxGetApplicationDirectory();
    pManager->LoadPluginsDirectory(lPath.Buffer());
#endif

    //Create an FBX scene. This object holds most objects imported/exported from/to files.
    pScene = FbxScene::Create(pManager, "My Scene");
    if (!pScene) {
        FBXSDK_printf("Error: Unable to create FBX scene!\n");
        exit(1);
    }
}
static void DestroySdkObjects(FbxManager* pManager, bool pExitStatus) {
    //Delete the FBX Manager. All the objects that have been allocated using the FBX Manager and that haven't been explicitly destroyed are also automatically destroyed.
    if (pManager)
        pManager->Destroy();
    if (pExitStatus)
        FBXSDK_printf("Program Success!\n");
}
static bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename) {
    int lFileMajor, lFileMinor, lFileRevision;
    int lSDKMajor, lSDKMinor, lSDKRevision;
    //int lFileFormat = -1;
    int lAnimStackCount;
    bool lStatus;
    char lPassword[1024];

    // Get the file version number generate by the FBX SDK.
    FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);

    // Create an importer.
    FbxImporter* lImporter = FbxImporter::Create(pManager, "");

    // Initialize the importer by providing a filename.
    const bool lImportStatus = lImporter->Initialize(pFilename, -1, pManager->GetIOSettings());
    lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

    if (!lImportStatus) {
        FbxString error = lImporter->GetStatus().GetErrorString();
        FBXSDK_printf("Call to FbxImporter::Initialize() failed.\n");
        FBXSDK_printf("Error returned: %s\n\n", error.Buffer());

        if (lImporter->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion) {
            FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);
            FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor,
                          lFileRevision);
        }

        return false;
    }

    FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);

    if (lImporter->IsFBX()) {
        FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor,
                      lFileRevision);

        // From this point, it is possible to access animation stack information without
        // the expense of loading the entire file.

        FBXSDK_printf("Animation Stack Information\n");

        lAnimStackCount = lImporter->GetAnimStackCount();

        FBXSDK_printf("    Number of Animation Stacks: %d\n", lAnimStackCount);
        FBXSDK_printf("    Current Animation Stack: \"%s\"\n", lImporter->GetActiveAnimStackName().Buffer());
        FBXSDK_printf("\n");

        for (int i = 0; i < lAnimStackCount; i++) {
            FbxTakeInfo* lTakeInfo = lImporter->GetTakeInfo(i);

            FBXSDK_printf("    Animation Stack %d\n", i);
            FBXSDK_printf("         Name: \"%s\"\n", lTakeInfo->mName.Buffer());
            FBXSDK_printf("         Description: \"%s\"\n", lTakeInfo->mDescription.Buffer());

            // Change the value of the import name if the animation stack should be imported
            // under a different name.
            FBXSDK_printf("         Import Name: \"%s\"\n", lTakeInfo->mImportName.Buffer());

            // Set the value of the import state to false if the animation stack should be not
            // be imported.
            FBXSDK_printf("         Import State: %s\n", lTakeInfo->mSelect ? "true" : "false");
            FBXSDK_printf("\n");
        }

        // Set the import states. By default, the import states are always set to
        // true. The code below shows how to change these states.
        IOS_REF.SetBoolProp(IMP_FBX_MATERIAL, true);
        IOS_REF.SetBoolProp(IMP_FBX_TEXTURE, true);
        IOS_REF.SetBoolProp(IMP_FBX_LINK, true);
        IOS_REF.SetBoolProp(IMP_FBX_SHAPE, true);
        IOS_REF.SetBoolProp(IMP_FBX_GOBO, true);
        IOS_REF.SetBoolProp(IMP_FBX_ANIMATION, true);
        IOS_REF.SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
    }

    // Import the scene.
    lStatus = lImporter->Import(pScene);
    if (lStatus == false && lImporter->GetStatus() == FbxStatus::ePasswordError) {
        FBXSDK_printf("Please enter password: ");

        lPassword[0] = '\0';

        FBXSDK_CRT_SECURE_NO_WARNING_BEGIN
        scanf("%s", lPassword);
        FBXSDK_CRT_SECURE_NO_WARNING_END

        FbxString lString(lPassword);

        IOS_REF.SetStringProp(IMP_FBX_PASSWORD, lString);
        IOS_REF.SetBoolProp(IMP_FBX_PASSWORD_ENABLE, true);

        lStatus = lImporter->Import(pScene);

        if (lStatus == false && lImporter->GetStatus() == FbxStatus::ePasswordError) {
            FBXSDK_printf("\nPassword is wrong, import aborted.\n");
        }
    }

    if (!lStatus || (lImporter->GetStatus() != FbxStatus::eSuccess)) {
        FBXSDK_printf("********************************************************************************\n");
        if (lStatus) {
            FBXSDK_printf("WARNING:\n");
            FBXSDK_printf("   The importer was able to read the file but with errors.\n");
            FBXSDK_printf("   Loaded scene may be incomplete.\n\n");
        } else {
            FBXSDK_printf("Importer failed to load the file!\n\n");
        }

        if (lImporter->GetStatus() != FbxStatus::eSuccess)
            FBXSDK_printf("   Last error message: %s\n", lImporter->GetStatus().GetErrorString());

        FbxArray<FbxString*> history;
        lImporter->GetStatus().GetErrorStringHistory(history);
        if (history.GetCount() > 1) {
            FBXSDK_printf("   Error history stack:\n");
            for (int i = 0; i < history.GetCount(); i++) {
                FBXSDK_printf("      %s\n", history[i]->Buffer());
            }
        }
        FbxArrayDelete<FbxString*>(history);
        FBXSDK_printf("********************************************************************************\n");
    }

    // Destroy the importer.
    lImporter->Destroy();

    return lStatus;
}
static void DisplayMetaData(FbxScene* pScene) {
    FbxDocumentInfo* sceneInfo = pScene->GetSceneInfo();
    if (sceneInfo) {
        FBXSDK_printf("\n\n--------------------\nMeta-Data\n--------------------\n\n");
        FBXSDK_printf("    Title: %s\n", sceneInfo->mTitle.Buffer());
        FBXSDK_printf("    Subject: %s\n", sceneInfo->mSubject.Buffer());
        FBXSDK_printf("    Author: %s\n", sceneInfo->mAuthor.Buffer());
        FBXSDK_printf("    Keywords: %s\n", sceneInfo->mKeywords.Buffer());
        FBXSDK_printf("    Revision: %s\n", sceneInfo->mRevision.Buffer());
        FBXSDK_printf("    Comment: %s\n", sceneInfo->mComment.Buffer());

        FbxThumbnail* thumbnail = sceneInfo->GetSceneThumbnail();
        if (thumbnail) {
            FBXSDK_printf("    Thumbnail:\n");

            switch (thumbnail->GetDataFormat()) {
                case FbxThumbnail::eRGB_24:
                    FBXSDK_printf("        Format: RGB\n");
                    break;
                case FbxThumbnail::eRGBA_32:
                    FBXSDK_printf("        Format: RGBA\n");
                    break;
            }

            switch (thumbnail->GetSize()) {
                default:
                    break;
                case FbxThumbnail::eNotSet:
                    FBXSDK_printf("        Size: no dimensions specified (%ld bytes)\n", thumbnail->GetSizeInBytes());
                    break;
                case FbxThumbnail::e64x64:
                    FBXSDK_printf("        Size: 64 x 64 pixels (%ld bytes)\n", thumbnail->GetSizeInBytes());
                    break;
                case FbxThumbnail::e128x128:
                    FBXSDK_printf("        Size: 128 x 128 pixels (%ld bytes)\n", thumbnail->GetSizeInBytes());
            }
        }
    }
}
static void DisplayMarker(FbxNode* pNode) {
    FbxMarker* lMarker = (FbxMarker*)pNode->GetNodeAttribute();
    FbxString lString;

    DisplayString("Marker Name: ", (char*)pNode->GetName());
    DisplayMetaDataConnections(lMarker);

    // Type
    lString = "    Marker Type: ";
    switch (lMarker->GetType()) {
        case FbxMarker::eStandard:
            lString += "Standard";
            break;
        case FbxMarker::eOptical:
            lString += "Optical";
            break;
        case FbxMarker::eEffectorIK:
            lString += "IK Effector";
            break;
        case FbxMarker::eEffectorFK:
            lString += "FK Effector";
            break;
    }
    DisplayString(lString.Buffer());

    // Look
    lString = "    Marker Look: ";
    switch (lMarker->Look.Get()) {
        default:
            break;
        case FbxMarker::eCube:
            lString += "Cube";
            break;
        case FbxMarker::eHardCross:
            lString += "Hard Cross";
            break;
        case FbxMarker::eLightCross:
            lString += "Light Cross";
            break;
        case FbxMarker::eSphere:
            lString += "Sphere";
            break;
    }
    DisplayString(lString.Buffer());

    // Size
    lString = FbxString("    Size: ") + FbxString(lMarker->Size.Get());
    DisplayString(lString.Buffer());

    // Color
    FbxDouble3 c = lMarker->Color.Get();
    FbxColor color(c[0], c[1], c[2]);
    DisplayColor("    Color: ", color);

    // IKPivot
    Display3DVector("    IKPivot: ", lMarker->IKPivot.Get());
}
static void DisplaySkeleton(FbxNode* pNode) {
    FbxSkeleton* lSkeleton = (FbxSkeleton*)pNode->GetNodeAttribute();

    DisplayString("Skeleton Name: ", (char*)pNode->GetName());
    DisplayMetaDataConnections(lSkeleton);

    const char* lSkeletonTypes[] = { "Root", "Limb", "Limb Node", "Effector" };

    DisplayString("    Type: ", lSkeletonTypes[lSkeleton->GetSkeletonType()]);

    if (lSkeleton->GetSkeletonType() == FbxSkeleton::eLimb) {
        DisplayDouble("    Limb Length: ", lSkeleton->LimbLength.Get());
    } else if (lSkeleton->GetSkeletonType() == FbxSkeleton::eLimbNode) {
        DisplayDouble("    Limb Node Size: ", lSkeleton->Size.Get());
    } else if (lSkeleton->GetSkeletonType() == FbxSkeleton::eRoot) {
        DisplayDouble("    Limb Root Size: ", lSkeleton->Size.Get());
    }

    DisplayColor("    Color: ", lSkeleton->GetLimbNodeColor());
}
static void DisplayMaterial(FbxGeometry* pGeometry, ECS::StaticMesh& InMesh) {
    int lMaterialCount = 0;
    FbxNode* lNode = NULL;
    if (pGeometry) {
        lNode = pGeometry->GetNode();
        if (lNode)
            lMaterialCount = lNode->GetMaterialCount();
    }

    if (lMaterialCount > 0) {
        FbxPropertyT<FbxDouble3> lKFbxDouble3;
        FbxPropertyT<FbxDouble> lKFbxDouble1;
        FbxColor theColor;

        for (int lCount = 0; lCount < lMaterialCount; lCount++) {
            DisplayInt("        Material ", lCount);

            FbxSurfaceMaterial* lMaterial = lNode->GetMaterial(lCount);

            DisplayString("            Name: \"", (char*)lMaterial->GetName(), "\"");

            //Get the implementation to see if it's a hardware shader.
            const FbxImplementation* lImplementation = LookForImplementation(lMaterial);
            if (lImplementation) {
                //Now we have a hardware shader, let's read it
                DisplayString("            Language: ", lImplementation->Language.Get().Buffer());
                DisplayString("            LanguageVersion: ", lImplementation->LanguageVersion.Get().Buffer());
                DisplayString("            RenderName: ", lImplementation->RenderName.Buffer());
                DisplayString("            RenderAPI: ", lImplementation->RenderAPI.Get().Buffer());
                DisplayString("            RenderAPIVersion: ", lImplementation->RenderAPIVersion.Get().Buffer());

                const FbxBindingTable* lRootTable = lImplementation->GetRootTable();
                FbxString lFileName = lRootTable->DescAbsoluteURL.Get();
                FbxString lTechniqueName = lRootTable->DescTAG.Get();


                const FbxBindingTable* lTable = lImplementation->GetRootTable();
                size_t lEntryNum = lTable->GetEntryCount();

                for (int i = 0; i < (int)lEntryNum; ++i) {
                    const FbxBindingTableEntry& lEntry = lTable->GetEntry(i);
                    const char* lEntrySrcType = lEntry.GetEntryType(true);
                    FbxProperty lFbxProp;


                    FbxString lTest = lEntry.GetSource();
                    DisplayString("            Entry: ", lTest.Buffer());
                    
                    if (strcmp(FbxPropertyEntryView::sEntryType, lEntrySrcType) == 0) {
                        lFbxProp = lMaterial->FindPropertyHierarchical(lEntry.GetSource());
                        if (!lFbxProp.IsValid()) {
                            lFbxProp = lMaterial->RootProperty.FindHierarchical(lEntry.GetSource());
                        }


                    } else if (strcmp(FbxConstantEntryView::sEntryType, lEntrySrcType) == 0) {
                        lFbxProp = lImplementation->GetConstants().FindHierarchical(lEntry.GetSource());
                    }
                    if (lFbxProp.IsValid()) {
                        if (lFbxProp.GetSrcObjectCount<FbxTexture>() > 0) {
                            //do what you want with the textures
                            for (int j = 0; j < lFbxProp.GetSrcObjectCount<FbxFileTexture>(); ++j) {
                                FbxFileTexture* lTex = lFbxProp.GetSrcObject<FbxFileTexture>(j);
                                DisplayString("           File Texture: ", lTex->GetFileName());
                            }
                            for (int j = 0; j < lFbxProp.GetSrcObjectCount<FbxLayeredTexture>(); ++j) {
                                FbxLayeredTexture* lTex = lFbxProp.GetSrcObject<FbxLayeredTexture>(j);
                                DisplayString("        Layered Texture: ", lTex->GetName());
                            }
                            for (int j = 0; j < lFbxProp.GetSrcObjectCount<FbxProceduralTexture>(); ++j) {
                                FbxProceduralTexture* lTex = lFbxProp.GetSrcObject<FbxProceduralTexture>(j);
                                DisplayString("     Procedural Texture: ", lTex->GetName());
                            }
                        } else {
                            FbxDataType lFbxType = lFbxProp.GetPropertyDataType();
                            FbxString blah = lFbxType.GetName();
                            if (FbxBoolDT == lFbxType) {
                                DisplayBool("                Bool: ", lFbxProp.Get<FbxBool>());
                            } else if (FbxIntDT == lFbxType || FbxEnumDT == lFbxType) {
                                DisplayInt("                Int: ", lFbxProp.Get<FbxInt>());
                            } else if (FbxFloatDT == lFbxType) {
                                DisplayDouble("                Float: ", lFbxProp.Get<FbxFloat>());

                            } else if (FbxDoubleDT == lFbxType) {
                                DisplayDouble("                Double: ", lFbxProp.Get<FbxDouble>());
                            } else if (FbxStringDT == lFbxType || FbxUrlDT == lFbxType || FbxXRefUrlDT == lFbxType) {
                                DisplayString("                String: ", lFbxProp.Get<FbxString>().Buffer());
                            } else if (FbxDouble2DT == lFbxType) {
                                FbxDouble2 lDouble2 = lFbxProp.Get<FbxDouble2>();
                                FbxVector2 lVect;
                                lVect[0] = lDouble2[0];
                                lVect[1] = lDouble2[1];

                                Display2DVector("                2D vector: ", lVect);
                            } else if (FbxDouble3DT == lFbxType || FbxColor3DT == lFbxType) {
                                FbxDouble3 lDouble3 = lFbxProp.Get<FbxDouble3>();
                                FbxVector4 lVect;
                                lVect[0] = lDouble3[0];
                                lVect[1] = lDouble3[1];
                                lVect[2] = lDouble3[2];
                                Display3DVector("                3D vector: ", lVect);
								if (lTest.Lower().Find("basecolor") >= 0)
								{
                                    InMesh.mDiffuseColor.x = lVect[0];
									InMesh.mDiffuseColor.y = lVect[1];
									InMesh.mDiffuseColor.z = lVect[2];
								}
                            }

                            else if (FbxDouble4DT == lFbxType || FbxColor4DT == lFbxType) {
                                FbxDouble4 lDouble4 = lFbxProp.Get<FbxDouble4>();
                                FbxVector4 lVect;
                                lVect[0] = lDouble4[0];
                                lVect[1] = lDouble4[1];
                                lVect[2] = lDouble4[2];
                                lVect[3] = lDouble4[3];
                                Display4DVector("                4D vector: ", lVect);
                            } else if (FbxDouble4x4DT == lFbxType) {
                                FbxDouble4x4 lDouble44 = lFbxProp.Get<FbxDouble4x4>();
                                for (int j = 0; j < 4; ++j) {

                                    FbxVector4 lVect;
                                    lVect[0] = lDouble44[j][0];
                                    lVect[1] = lDouble44[j][1];
                                    lVect[2] = lDouble44[j][2];
                                    lVect[3] = lDouble44[j][3];
                                    Display4DVector("                4x4D vector: ", lVect);
                                }
                            }
                        }
                    }
                }
            } else if (lMaterial->GetClassId().Is(FbxSurfacePhong::ClassId)) {
                // We found a Phong material.  Display its properties.

                // Display the Ambient Color
                lKFbxDouble3 = ((FbxSurfacePhong*)lMaterial)->Ambient;
                theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
                DisplayColor("            Ambient: ", theColor);

                // Display the Diffuse Color
                lKFbxDouble3 = ((FbxSurfacePhong*)lMaterial)->Diffuse;
                theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
                DisplayColor("            Diffuse: ", theColor);

                // Display the Specular Color (unique to Phong materials)
                lKFbxDouble3 = ((FbxSurfacePhong*)lMaterial)->Specular;
                theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
                DisplayColor("            Specular: ", theColor);

                // Display the Emissive Color
                lKFbxDouble3 = ((FbxSurfacePhong*)lMaterial)->Emissive;
                theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
                DisplayColor("            Emissive: ", theColor);

                //Opacity is Transparency factor now
                lKFbxDouble1 = ((FbxSurfacePhong*)lMaterial)->TransparencyFactor;
                DisplayDouble("            Opacity: ", 1.0 - lKFbxDouble1.Get());

                // Display the Shininess
                lKFbxDouble1 = ((FbxSurfacePhong*)lMaterial)->Shininess;
                DisplayDouble("            Shininess: ", lKFbxDouble1.Get());

                // Display the Reflectivity
                lKFbxDouble1 = ((FbxSurfacePhong*)lMaterial)->ReflectionFactor;
                DisplayDouble("            Reflectivity: ", lKFbxDouble1.Get());
            } else if (lMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId)) {
                // We found a Lambert material. Display its properties.
                // Display the Ambient Color
                lKFbxDouble3 = ((FbxSurfaceLambert*)lMaterial)->Ambient;
                theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
                DisplayColor("            Ambient: ", theColor);

                // Display the Diffuse Color
                lKFbxDouble3 = ((FbxSurfaceLambert*)lMaterial)->Diffuse;
                theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
                DisplayColor("            Diffuse: ", theColor);

                // Display the Emissive
                lKFbxDouble3 = ((FbxSurfaceLambert*)lMaterial)->Emissive;
                theColor.Set(lKFbxDouble3.Get()[0], lKFbxDouble3.Get()[1], lKFbxDouble3.Get()[2]);
                DisplayColor("            Emissive: ", theColor);

                // Display the Opacity
                lKFbxDouble1 = ((FbxSurfaceLambert*)lMaterial)->TransparencyFactor;
                DisplayDouble("            Opacity: ", 1.0 - lKFbxDouble1.Get());
            } else
                DisplayString("Unknown type of Material");

            FbxPropertyT<FbxString> lString;
            lString = lMaterial->ShadingModel;
            DisplayString("            Shading Model: ", lString.Get().Buffer());
            DisplayString("");
        }
    }
}
static void DisplayTextureInfo(FbxTexture* pTexture, int pBlendMode) {
    FbxFileTexture* lFileTexture = FbxCast<FbxFileTexture>(pTexture);
    FbxProceduralTexture* lProceduralTexture = FbxCast<FbxProceduralTexture>(pTexture);

    DisplayString("            Name: \"", (char*)pTexture->GetName(), "\"");
    if (lFileTexture) {
        DisplayString("            Type: File Texture");
        DisplayString("            File Name: \"", (char*)lFileTexture->GetFileName(), "\"");
    } else if (lProceduralTexture) {
        DisplayString("            Type: Procedural Texture");
    }
    DisplayDouble("            Scale U: ", pTexture->GetScaleU());
    DisplayDouble("            Scale V: ", pTexture->GetScaleV());
    DisplayDouble("            Translation U: ", pTexture->GetTranslationU());
    DisplayDouble("            Translation V: ", pTexture->GetTranslationV());
    DisplayBool("            Swap UV: ", pTexture->GetSwapUV());
    DisplayDouble("            Rotation U: ", pTexture->GetRotationU());
    DisplayDouble("            Rotation V: ", pTexture->GetRotationV());
    DisplayDouble("            Rotation W: ", pTexture->GetRotationW());

    const char* lAlphaSources[] = { "None", "RGB Intensity", "Black" };

    DisplayString("            Alpha Source: ", lAlphaSources[pTexture->GetAlphaSource()]);
    DisplayDouble("            Cropping Left: ", pTexture->GetCroppingLeft());
    DisplayDouble("            Cropping Top: ", pTexture->GetCroppingTop());
    DisplayDouble("            Cropping Right: ", pTexture->GetCroppingRight());
    DisplayDouble("            Cropping Bottom: ", pTexture->GetCroppingBottom());

    const char* lMappingTypes[] = { "Null", "Planar", "Spherical", "Cylindrical", "Box", "Face", "UV", "Environment" };

    DisplayString("            Mapping Type: ", lMappingTypes[pTexture->GetMappingType()]);

    if (pTexture->GetMappingType() == FbxTexture::ePlanar) {
        const char* lPlanarMappingNormals[] = { "X", "Y", "Z" };

        DisplayString("            Planar Mapping Normal: ", lPlanarMappingNormals[pTexture->GetPlanarMappingNormal()]);
    }

    const char* lBlendModes[] = { "Translucent",  "Additive",  "Modulate",   "Modulate2",  "Over",
                                  "Normal",       "Dissolve",  "Darken",     "ColorBurn",  "LinearBurn",
                                  "DarkerColor",  "Lighten",   "Screen",     "ColorDodge", "LinearDodge",
                                  "LighterColor", "SoftLight", "HardLight",  "VividLight", "LinearLight",
                                  "PinLight",     "HardMix",   "Difference", "Exclusion",  "Substract",
                                  "Divide",       "Hue",       "Saturation", "Color",      "Luminosity",
                                  "Overlay" };

    if (pBlendMode >= 0)
        DisplayString("            Blend Mode: ", lBlendModes[pBlendMode]);
    DisplayDouble("            Alpha: ", pTexture->GetDefaultAlpha());

    if (lFileTexture) {
        const char* lMaterialUses[] = { "Model Material", "Default Material" };
        DisplayString("            Material Use: ", lMaterialUses[lFileTexture->GetMaterialUse()]);
    }

    const char* pTextureUses[] = {
        "Standard", "Shadow Map", "Light Map", "Spherical Reflexion Map", "Sphere Reflexion Map", "Bump Normal Map"
    };

    DisplayString("            Texture Use: ", pTextureUses[pTexture->GetTextureUse()]);
    DisplayString("");
}
static void FindAndDisplayTextureInfoByProperty(FbxProperty pProperty, bool& pDisplayHeader, int pMaterialIndex) {

    if (pProperty.IsValid()) {
        int lTextureCount = pProperty.GetSrcObjectCount<FbxTexture>();

        for (int j = 0; j < lTextureCount; ++j) {
            //Here we have to check if it's layeredtextures, or just textures:
            FbxLayeredTexture* lLayeredTexture = pProperty.GetSrcObject<FbxLayeredTexture>(j);
            if (lLayeredTexture) {
                DisplayInt("    Layered Texture: ", j);
                int lNbTextures = lLayeredTexture->GetSrcObjectCount<FbxTexture>();
                for (int k = 0; k < lNbTextures; ++k) {
                    FbxTexture* lTexture = lLayeredTexture->GetSrcObject<FbxTexture>(k);
                    if (lTexture) {

                        if (pDisplayHeader) {
                            DisplayInt("    Textures connected to Material ", pMaterialIndex);
                            pDisplayHeader = false;
                        }

                        //NOTE the blend mode is ALWAYS on the LayeredTexture and NOT the one on the texture.
                        //Why is that?  because one texture can be shared on different layered textures and might
                        //have different blend modes.

                        FbxLayeredTexture::EBlendMode lBlendMode;
                        lLayeredTexture->GetTextureBlendMode(k, lBlendMode);
                        DisplayString("    Textures for ", pProperty.GetName());
                        DisplayInt("        Texture ", k);
                        DisplayTextureInfo(lTexture, (int)lBlendMode);
                    }
                }
            } else {
                //no layered texture simply get on the property
                FbxTexture* lTexture = pProperty.GetSrcObject<FbxTexture>(j);
                if (lTexture) {
                    //display connected Material header only at the first time
                    if (pDisplayHeader) {
                        DisplayInt("    Textures connected to Material ", pMaterialIndex);
                        pDisplayHeader = false;
                    }

                    DisplayString("    Textures for ", pProperty.GetName());
                    DisplayInt("        Texture ", j);
                    DisplayTextureInfo(lTexture, -1);
                }
            }
        }
    }  //end if pProperty
}

static void DisplayTexture(FbxGeometry* pGeometry) {
    int lMaterialIndex;
    FbxProperty lProperty;
    if (pGeometry->GetNode() == NULL)
        return;
    int lNbMat = pGeometry->GetNode()->GetSrcObjectCount<FbxSurfaceMaterial>();
    for (lMaterialIndex = 0; lMaterialIndex < lNbMat; lMaterialIndex++) {
        FbxSurfaceMaterial* lMaterial = pGeometry->GetNode()->GetSrcObject<FbxSurfaceMaterial>(lMaterialIndex);
        bool lDisplayHeader = true;

        //go through all the possible textures
        if (lMaterial) {

            int lTextureIndex;
            FBXSDK_FOR_EACH_TEXTURE(lTextureIndex) {
                lProperty = lMaterial->FindProperty(FbxLayerElement::sTextureChannelNames[lTextureIndex]);
                FindAndDisplayTextureInfoByProperty(lProperty, lDisplayHeader, lMaterialIndex);
            }

        }  //end if(lMaterial)

    }  // end for lMaterialIndex
}

static void DisplayLink(FbxGeometry* pGeometry) {
    //Display cluster now

    //int i, lLinkCount;
    //FbxCluster* lLink;

    int i, j;
    int lSkinCount = 0;
    int lClusterCount = 0;
    FbxCluster* lCluster;

    lSkinCount = pGeometry->GetDeformerCount(FbxDeformer::eSkin);


    //lLinkCount = pGeometry->GetLinkCount();
    for (i = 0; i != lSkinCount; ++i) {
        lClusterCount = ((FbxSkin*)pGeometry->GetDeformer(i, FbxDeformer::eSkin))->GetClusterCount();
        for (j = 0; j != lClusterCount; ++j) {
            DisplayInt("    Cluster ", i);

            lCluster = ((FbxSkin*)pGeometry->GetDeformer(i, FbxDeformer::eSkin))->GetCluster(j);
            //lLink = pGeometry->GetLink(i);

            const char* lClusterModes[] = { "Normalize", "Additive", "Total1" };

            DisplayString("    Mode: ", lClusterModes[lCluster->GetLinkMode()]);

            if (lCluster->GetLink() != NULL) {
                DisplayString("        Name: ", (char*)lCluster->GetLink()->GetName());
            }

            FbxString lString1 = "        Link Indices: ";
            FbxString lString2 = "        Weight Values: ";

            int k, lIndexCount = lCluster->GetControlPointIndicesCount();
            int* lIndices = lCluster->GetControlPointIndices();
            double* lWeights = lCluster->GetControlPointWeights();

            for (k = 0; k < lIndexCount; k++) {
                lString1 += lIndices[k];
                lString2 += (float)lWeights[k];

                if (k < lIndexCount - 1) {
                    lString1 += ", ";
                    lString2 += ", ";
                }
            }

            lString1 += "\n";
            lString2 += "\n";

            FBXSDK_printf(lString1);
            FBXSDK_printf(lString2);

            DisplayString("");

            FbxAMatrix lMatrix;

            lMatrix = lCluster->GetTransformMatrix(lMatrix);
            Display3DVector("        Transform Translation: ", lMatrix.GetT());
            Display3DVector("        Transform Rotation: ", lMatrix.GetR());
            Display3DVector("        Transform Scaling: ", lMatrix.GetS());

            lMatrix = lCluster->GetTransformLinkMatrix(lMatrix);
            Display3DVector("        Transform Link Translation: ", lMatrix.GetT());
            Display3DVector("        Transform Link Rotation: ", lMatrix.GetR());
            Display3DVector("        Transform Link Scaling: ", lMatrix.GetS());

            if (lCluster->GetAssociateModel() != NULL) {
                lMatrix = lCluster->GetTransformAssociateModelMatrix(lMatrix);
                DisplayString("        Associate Model: ", (char*)lCluster->GetAssociateModel()->GetName());
                Display3DVector("        Associate Model Translation: ", lMatrix.GetT());
                Display3DVector("        Associate Model Rotation: ", lMatrix.GetR());
                Display3DVector("        Associate Model Scaling: ", lMatrix.GetS());
            }

            DisplayString("");
        }
    }
}
void DisplayElementData(const FbxString& pHeader, const FbxVector4& pData, int index = -1) {
    FbxString desc(pHeader);
    if (index != -1) {
        FbxString num = FbxString(" [") + index + "]: ";
        desc.FindAndReplace(":", num.Buffer());
    }
    Display3DVector(desc.Buffer(), pData);
}

void DisplayElementData(const FbxString& pHeader, const FbxVector2& pData, int index = -1) {
    FbxString desc(pHeader);
    if (index != -1) {
        FbxString num = FbxString(" [") + index + "]: ";
        desc.FindAndReplace(":", num.Buffer());
    }
    Display2DVector(desc.Buffer(), pData);
}

void DisplayElementData(const FbxString& pHeader, const FbxColor& pData, int index = -1) {
    FbxString desc(pHeader);
    if (index != -1) {
        FbxString num = FbxString(" [") + index + "]: ";
        desc.FindAndReplace(":", num.Buffer());
    }
    DisplayColor(desc.Buffer(), pData);
}
static void FillHeaderBasedOnElementType(FbxLayerElement::EType pComponent, FbxString& pHeader) {
    switch (pComponent) {
        case FbxLayerElement::eNormal:
            pHeader = "        Normal: ";
            break;
        case FbxLayerElement::eBiNormal:
            pHeader = "        BiNormal: ";
            break;
        case FbxLayerElement::eTangent:
            pHeader = "        Tangent: ";
            break;
        case FbxLayerElement::eUV:
            pHeader = "        UV: ";
            break;
        case FbxLayerElement::eVertexColor:
            pHeader = "        Vertex Color: ";
            break;
        default:
            pHeader = "        Unsupported element: ";
            break;
    }
}
template<class T>
void DisplayLayerElement(FbxLayerElement::EType pComponent, const FbxLayer* pShapeLayer, const FbxMesh* pMesh) {
    const FbxLayerElement* pLayerElement = pShapeLayer->GetLayerElementOfType(pComponent);
    if (pLayerElement) {
        FbxLayerElementTemplate<T>* pLayerElementTemplate = ((FbxLayerElementTemplate<T>*)pLayerElement);
        FbxLayerElementArrayTemplate<T>& pLayerElementArray = pLayerElementTemplate->GetDirectArray();
        FbxString header;
        FillHeaderBasedOnElementType(pComponent, header);
        int lPolygonCount = pMesh->GetPolygonCount();
        int lPolynodeIndex = 0;
        for (int i = 0; i < lPolygonCount; ++i) {
            int lPolygonSize = pMesh->GetPolygonSize(i);
            for (int j = 0; j < lPolygonSize; ++j, ++lPolynodeIndex) {
                int lControlPointIndex = pMesh->GetPolygonVertex(i, j);
                switch (pLayerElementTemplate->GetMappingMode()) {
                    default:
                        break;
                    case FbxGeometryElement::eByControlPoint: {
                        switch (pLayerElementTemplate->GetReferenceMode()) {
                            case FbxGeometryElement::eDirect:
                                DisplayElementData(header, pLayerElementArray.GetAt(lControlPointIndex),
                                                   lControlPointIndex);
                                break;
                            case FbxGeometryElement::eIndexToDirect: {
                                FbxLayerElementArrayTemplate<int>& pLayerElementIndexArray =
                                        pLayerElementTemplate->GetIndexArray();
                                int id = pLayerElementIndexArray.GetAt(lControlPointIndex);
                                if (id > 0) {
                                    DisplayElementData(header, pLayerElementArray.GetAt(id), id);
                                }
                            } break;
                            default:
                                break;  // other reference modes not shown here!
                        };
                    }
                    case FbxGeometryElement::eByPolygonVertex: {
                        switch (pLayerElementTemplate->GetReferenceMode()) {
                            case FbxGeometryElement::eDirect:
                                DisplayElementData(header, pLayerElementArray.GetAt(lPolynodeIndex), lPolynodeIndex);
                                break;
                            case FbxGeometryElement::eIndexToDirect: {
                                FbxLayerElementArrayTemplate<int>& pLayerElementIndexArray =
                                        pLayerElementTemplate->GetIndexArray();
                                int id = pLayerElementIndexArray.GetAt(lPolynodeIndex);
                                if (id > 0) {
                                    DisplayElementData(header, pLayerElementArray.GetAt(id), id);
                                }
                            } break;
                            default:
                                break;  // other reference modes not shown here!
                        }
                    } break;
                }
            }
        }
    }
}

static void DisplayShapeLayerElements(const FbxShape* pShape, const FbxMesh* pMesh) {
    int lShapeControlPointsCount = pShape->GetControlPointsCount();
    int lMeshControlPointsCount = pMesh->GetControlPointsCount();
    bool lValidLayerElementSource = pShape && pMesh && lShapeControlPointsCount && lMeshControlPointsCount;
    if (lValidLayerElementSource) {
        if (lShapeControlPointsCount == lMeshControlPointsCount) {
            // Display control points that are different from the mesh
            FbxVector4* lShapeControlPoint = pShape->GetControlPoints();
            FbxVector4* lMeshControlPoint = pMesh->GetControlPoints();
            for (int j = 0; j < lShapeControlPointsCount; j++) {
                FbxVector4 delta = lShapeControlPoint[j] - lMeshControlPoint[j];
                if (!FbxEqual(delta, FbxZeroVector4)) {
                    FbxString header("        Control Point: ");
                    DisplayElementData(header, lShapeControlPoint[j], j);
                }
            }
        }

        int lLayerCount = pShape->GetLayerCount();
        for (int i = 0; i < lLayerCount; ++i) {
            const FbxLayer* pLayer = pShape->GetLayer(i);
            DisplayLayerElement<FbxVector4>(FbxLayerElement::eNormal, pLayer, pMesh);
            DisplayLayerElement<FbxColor>(FbxLayerElement::eVertexColor, pLayer, pMesh);
            DisplayLayerElement<FbxVector4>(FbxLayerElement::eTangent, pLayer, pMesh);
            DisplayLayerElement<FbxColor>(FbxLayerElement::eBiNormal, pLayer, pMesh);
            DisplayLayerElement<FbxColor>(FbxLayerElement::eUV, pLayer, pMesh);
        }
    }
}


static void DisplayShape(FbxGeometry* pGeometry) {
    int lBlendShapeCount, lBlendShapeChannelCount, lTargetShapeCount;
    FbxBlendShape* lBlendShape;
    FbxBlendShapeChannel* lBlendShapeChannel;
    FbxShape* lShape;

    lBlendShapeCount = pGeometry->GetDeformerCount(FbxDeformer::eBlendShape);

    for (int lBlendShapeIndex = 0; lBlendShapeIndex < lBlendShapeCount; ++lBlendShapeIndex) {
        lBlendShape = (FbxBlendShape*)pGeometry->GetDeformer(lBlendShapeIndex, FbxDeformer::eBlendShape);
        DisplayString("    BlendShape ", (char*)lBlendShape->GetName());

        lBlendShapeChannelCount = lBlendShape->GetBlendShapeChannelCount();
        for (int lBlendShapeChannelIndex = 0; lBlendShapeChannelIndex < lBlendShapeChannelCount;
             ++lBlendShapeChannelIndex) {
            lBlendShapeChannel = lBlendShape->GetBlendShapeChannel(lBlendShapeChannelIndex);
            DisplayString("    BlendShapeChannel ", (char*)lBlendShapeChannel->GetName());
            DisplayDouble("    Default Deform Value: ", lBlendShapeChannel->DeformPercent.Get());

            lTargetShapeCount = lBlendShapeChannel->GetTargetShapeCount();
            for (int lTargetShapeIndex = 0; lTargetShapeIndex < lTargetShapeCount; ++lTargetShapeIndex) {
                lShape = lBlendShapeChannel->GetTargetShape(lTargetShapeIndex);
                DisplayString("    TargetShape ", (char*)lShape->GetName());

                if (pGeometry->GetAttributeType() == FbxNodeAttribute::eMesh) {
                    DisplayShapeLayerElements(lShape, FbxCast<FbxMesh>(pGeometry));
                } else {
                    int j, lControlPointsCount = lShape->GetControlPointsCount();
                    FbxVector4* lControlPoints = lShape->GetControlPoints();
                    FbxLayerElementArrayTemplate<FbxVector4>* lNormals = NULL;
                    bool lStatus = lShape->GetNormals(&lNormals);

                    for (j = 0; j < lControlPointsCount; j++) {
                        DisplayInt("        Control Point ", j);
                        Display3DVector("            Coordinates: ", lControlPoints[j]);

                        if (lStatus && lNormals && lNormals->GetCount() == lControlPointsCount) {
                            Display3DVector("            Normal Vector: ", lNormals->GetAt(j));
                        }
                    }
                }

                DisplayString("");
            }
        }
    }
}

static void DisplayCache(FbxGeometry* pGeometry) {
    int lVertexCacheDeformerCount = pGeometry->GetDeformerCount(FbxDeformer::eVertexCache);

    for (int i = 0; i < lVertexCacheDeformerCount; ++i) {
        FbxVertexCacheDeformer* lDeformer =
                static_cast<FbxVertexCacheDeformer*>(pGeometry->GetDeformer(i, FbxDeformer::eVertexCache));
        if (!lDeformer)
            continue;

        FbxCache* lCache = lDeformer->GetCache();
        if (!lCache)
            continue;

        if (lCache->OpenFileForRead()) {
            DisplayString("    Vertex Cache");
            int lChannelIndex = lCache->GetChannelIndex(lDeformer->Channel.Get());
            // skip normal channel
            if (lChannelIndex < 0)
                continue;

            FbxString lChnlName, lChnlInterp;

            FbxCache::EMCDataType lChnlType;
            FbxTime start, stop, rate;
            FbxCache::EMCSamplingType lChnlSampling;
            unsigned int lChnlSampleCount, lDataCount;

            lCache->GetChannelName(lChannelIndex, lChnlName);
            DisplayString("        Channel Name: ", lChnlName.Buffer());
            lCache->GetChannelDataType(lChannelIndex, lChnlType);
            switch (lChnlType) {
                case FbxCache::eUnknownData:
                    DisplayString("        Channel Type: Unknown Data");
                    break;
                case FbxCache::eDouble:
                    DisplayString("        Channel Type: Double");
                    break;
                case FbxCache::eDoubleArray:
                    DisplayString("        Channel Type: Double Array");
                    break;
                case FbxCache::eDoubleVectorArray:
                    DisplayString("        Channel Type: Double Vector Array");
                    break;
                case FbxCache::eInt32Array:
                    DisplayString("        Channel Type: Int32 Array");
                    break;
                case FbxCache::eFloatArray:
                    DisplayString("        Channel Type: Float Array");
                    break;
                case FbxCache::eFloatVectorArray:
                    DisplayString("        Channel Type: Float Vector Array");
                    break;
            }
            lCache->GetChannelInterpretation(lChannelIndex, lChnlInterp);
            DisplayString("        Channel Interpretation: ", lChnlInterp.Buffer());
            lCache->GetChannelSamplingType(lChannelIndex, lChnlSampling);
            DisplayInt("        Channel Sampling Type: ", lChnlSampling);
            lCache->GetAnimationRange(lChannelIndex, start, stop);
            lCache->GetChannelSamplingRate(lChannelIndex, rate);
            lCache->GetChannelSampleCount(lChannelIndex, lChnlSampleCount);
            DisplayInt("        Channel Sample Count: ", lChnlSampleCount);

            // Only display cache data if the data type is float vector array
            if (lChnlType != FbxCache::eFloatVectorArray)
                continue;

            if (lChnlInterp == "normals")
                DisplayString("        Normal Cache Data");
            else
                DisplayString("        Points Cache Data");
            float* lBuffer = NULL;
            unsigned int lBufferSize = 0;
            int lFrame = 0;
            for (FbxTime t = start; t <= stop; t += rate) {
                DisplayInt("            Frame ", lFrame);
                lCache->GetChannelPointCount(lChannelIndex, t, lDataCount);
                if (lBuffer == NULL) {
                    lBuffer = new float[lDataCount * 3];
                    lBufferSize = lDataCount * 3;
                } else if (lBufferSize < lDataCount * 3) {
                    delete[] lBuffer;
                    lBuffer = new float[lDataCount * 3];
                    lBufferSize = lDataCount * 3;
                } else
                    memset(lBuffer, 0, lBufferSize * sizeof(float));

                lCache->Read(lChannelIndex, t, lBuffer, lDataCount);
                if (lChnlInterp == "normals") {
                    // display normals cache data
                    // the normal data is per-polygon per-vertex. we can get the polygon vertex index
                    // from the index array of polygon vertex
                    FbxMesh* lMesh = (FbxMesh*)pGeometry;

                    if (lMesh == NULL) {
                        // Only Mesh can have normal cache data
                        continue;
                    }

                    DisplayInt("                Normal Count ", lDataCount);
                    int pi, j, lPolygonCount = lMesh->GetPolygonCount();
                    unsigned lNormalIndex = 0;
                    for (pi = 0; pi < lPolygonCount && lNormalIndex + 2 < lDataCount * 3; pi++) {
                        DisplayInt("                    Polygon ", pi);
                        DisplayString("                    Normals for Each Polygon Vertex: ");
                        int lPolygonSize = lMesh->GetPolygonSize(pi);
                        for (j = 0; j < lPolygonSize && lNormalIndex + 2 < lDataCount * 3; j++) {
                            FbxVector4 normal(lBuffer[lNormalIndex], lBuffer[lNormalIndex + 1],
                                              lBuffer[lNormalIndex + 2]);
                            Display3DVector("                       Normal Cache Data  ", normal);
                            lNormalIndex += 3;
                        }
                    }
                } else {
                    DisplayInt("               Points Count: ", lDataCount);
                    for (unsigned int j = 0; j < lDataCount * 3; j = j + 3) {
                        FbxVector4 points(lBuffer[j], lBuffer[j + 1], lBuffer[j + 2]);
                        Display3DVector("                   Points Cache Data: ", points);
                    }
                }

                lFrame++;
            }

            if (lBuffer != NULL) {
                delete[] lBuffer;
                lBuffer = NULL;
            }

            lCache->CloseFile();
        }
    }
}

void AssetLoader::FbxLoader::GetNodeGeometricTransform(FbxNode* pNode) {
    FbxVector4 lTmpVector;
    using namespace  DirectX::SimpleMath;
    FBXSDK_printf("    Geometric Transformations\n");
    auto& newMesh = mStaticMeshes[mStaticMeshes.size() - 1];

    //
    // Translation
    //
    lTmpVector = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
    FBXSDK_printf("        Translation: %f %f %f\n", lTmpVector[0], lTmpVector[1], lTmpVector[2]);
    newMesh.Translation = Vector3(lTmpVector[0], lTmpVector[1], lTmpVector[2]);
    //
    // Rotation
    //
    lTmpVector = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
    FBXSDK_printf("        Rotation:    %f %f %f\n", lTmpVector[0], lTmpVector[1], lTmpVector[2]);
    newMesh.Rotation = Vector3(lTmpVector[0], lTmpVector[1], lTmpVector[2]);

    //
    // Scaling
    //
    lTmpVector = pNode->GetGeometricScaling(FbxNode::eSourcePivot);
    FBXSDK_printf("        Scaling:     %f %f %f\n", lTmpVector[0], lTmpVector[1], lTmpVector[2]);
    newMesh.Scale = Vector3(lTmpVector[0], lTmpVector[1], lTmpVector[2]);
}


void AssetLoader::FbxLoader::DisplayMesh(FbxNode* pNode, const FbxAMatrix& globalPos) {
    FbxMesh* lMesh = (FbxMesh*)pNode->GetNodeAttribute();
    if (LoadStaticMesh(lMesh)) 
    {
        using namespace DirectX::SimpleMath;
        //load mesh successfully, now get transform info
        FBXSDK_printf("    Geometric Transformations\n");
        auto& newMesh = mStaticMeshes[mStaticMeshes.size() - 1];
        FbxVector4 lTmpVector;
        //
        // Translation
        //
        lTmpVector = globalPos.GetT();
        FBXSDK_printf("        Translation: %f %f %f\n", lTmpVector[0], lTmpVector[1], lTmpVector[2]);
        newMesh.Translation = Vector3(lTmpVector[0], lTmpVector[1], lTmpVector[2]);
        //
        // Rotation
        //
        lTmpVector = globalPos.GetR();
        FBXSDK_printf("        Rotation:    %f %f %f\n", lTmpVector[0], lTmpVector[1], lTmpVector[2]);
        newMesh.Rotation = Vector3(lTmpVector[0], lTmpVector[1], lTmpVector[2]);

        //
        // Scaling
        //
        lTmpVector = globalPos.GetS();
        FBXSDK_printf("        Scaling:     %f %f %f\n", lTmpVector[0], lTmpVector[1], lTmpVector[2]);
        newMesh.Scale = Vector3(lTmpVector[0], lTmpVector[1], lTmpVector[2]);
    }

    //DisplayString("Mesh Name: ", (char*)pNode->GetName());
    //DisplayMetaDataConnections(lMesh);
    //DisplayControlsPoints(lMesh);
    //DisplayPolygons(lMesh);
    //DisplayMaterialMapping(lMesh);
    //DisplayTexture(lMesh);
    //DisplayLink(lMesh);
    //DisplayShape(lMesh);
    //DisplayCache(lMesh);
}

FbxAMatrix AssetLoader::FbxLoader::GetGlobalPosition(FbxNode* pNode, const FbxTime& pTime, FbxPose* pPose,
                                                     FbxAMatrix* pParentGlobalPosition) {
    FbxAMatrix lGlobalPosition;
    bool lPositionFound = false;

    if (pPose) {
        Ensures(false);
        //int lNodeIndex = pPose->Find(pNode);
        //
        //if (lNodeIndex > -1) {
        //    // The bind pose is always a global matrix.
        //    // If we have a rest pose, we need to check if it is
        //    // stored in global or local space.
        //    if (pPose->IsBindPose() || !pPose->IsLocalMatrix(lNodeIndex)) {
        //        lGlobalPosition = GetPoseMatrix(pPose, lNodeIndex);
        //    } else {
        //        // We have a local matrix, we need to convert it to
        //        // a global space matrix.
        //        FbxAMatrix lParentGlobalPosition;
        //
        //        if (pParentGlobalPosition) {
        //            lParentGlobalPosition = *pParentGlobalPosition;
        //        } else {
        //            if (pNode->GetParent()) {
        //                lParentGlobalPosition = GetGlobalPosition(pNode->GetParent(), pTime, pPose);
        //            }
        //        }
        //
        //        FbxAMatrix lLocalPosition = GetPoseMatrix(pPose, lNodeIndex);
        //        lGlobalPosition = lParentGlobalPosition * lLocalPosition;
        //    }
        //
        //    lPositionFound = true;
        //}
    }

    if (!lPositionFound) {
        // There is no pose entry for that node, get the current global position instead.

        // Ideally this would use parent global position and local position to compute the global position.
        // Unfortunately the equation
        //    lGlobalPosition = pParentGlobalPosition * lLocalPosition
        // does not hold when inheritance type is other than "Parent" (RSrs).
        // To compute the parent rotation and scaling is tricky in the RrSs and Rrs cases.
        lGlobalPosition = pNode->EvaluateGlobalTransform(pTime);
    }

    return lGlobalPosition;
}

void AssetLoader::FbxLoader::LoadTextureMaterial(FbxScene* InScene, const std::string_view InFileName) {
    // Load the textures into GPU, only for file texture now
    const int lTextureCount = InScene->GetTextureCount();
    std::vector<int> dummyVec(lTextureCount, 0);
    for (auto i = 0 ; i < dummyVec.size();++i)
    {
        dummyVec[i] = i;
    }
    std::for_each(std::execution::seq, dummyVec.begin(), dummyVec.end(), [&](int& lTextureIndex)
        { 
			FbxTexture* lTexture = InScene->GetTexture(lTextureIndex);
			FbxFileTexture* lFileTexture = FbxCast<FbxFileTexture>(lTexture);
			if (lFileTexture && !lFileTexture->GetUserDataPtr()) {
				// Try to load the texture from absolute path
				const FbxString lFileName = lFileTexture->GetFileName();

				// Only TGA textures are supported now.
				//if (lFileName.Right(3).Upper() != "TGA") {
				//    FBXSDK_printf("Only TGA textures are supported now: %s\n", lFileName.Buffer());
				//    continue;
				//}

				int lTextureObject = 0;
				std::optional<TextureData*> newTexture = gStbTextureLoader->LoadTextureFromFile(lFileName.Buffer());
				if (newTexture.has_value())
				{
                    std::lock_guard<std::mutex> lock(mTextureMapMutext);
					mTextureMap[lTexture->GetName()] = newTexture.value();
				}

				const FbxString lAbsFbxFileName = FbxPathUtils::Resolve(InFileName.data());
				const FbxString lAbsFolderName = FbxPathUtils::GetFolderName(lAbsFbxFileName);
				if (!newTexture.has_value()) {
					// Load texture from relative file name (relative to FBX file)
					const FbxString lResolvedFileName =
						FbxPathUtils::Bind(lAbsFolderName, lFileTexture->GetRelativeFileName());
					std::optional<TextureData*> newTexture = gStbTextureLoader->LoadTextureFromFile(lResolvedFileName.Buffer());
				}

				if (!newTexture.has_value()) {
					// Load texture from file name only (relative to FBX file)
					const FbxString lTextureFileName = FbxPathUtils::GetFileName(lFileName);
					const FbxString lResolvedFileName = FbxPathUtils::Bind(lAbsFolderName, lTextureFileName);
					std::optional<TextureData*> newTexture = gStbTextureLoader->LoadTextureFromFile(lResolvedFileName.Buffer());
				}

				if (!newTexture.has_value()) {
					FBXSDK_printf("Failed to load texture file: %s\n", lFileName.Buffer());
				}

				//if (lStatus) {
				//    GLuint* lTextureName = new GLuint(lTextureObject);
				//    lFileTexture->SetUserDataPtr(lTextureName);
				//}
			}
        });
}

FbxAMatrix GetGeometry(FbxNode* pNode) {
    const FbxVector4 lT = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
    const FbxVector4 lR = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
    const FbxVector4 lS = pNode->GetGeometricScaling(FbxNode::eSourcePivot);

    return FbxAMatrix(lT, lR, lS);
}

void AssetLoader::FbxLoader::DisplayContent(FbxNode* pNode, FbxTime& pTime, FbxAnimLayer* pAnimLayer,
                                            FbxAMatrix& pParentGlobalPosition, FbxPose* pPose) {
    FbxNodeAttribute::EType lAttributeType;
    int i;
    FbxAMatrix lGlobalPosition = GetGlobalPosition(pNode, pTime, pPose, &pParentGlobalPosition);

    if (pNode->GetNodeAttribute() == NULL) {
        FBXSDK_printf("NULL Node Attribute\n\n");
    } else {
        lAttributeType = (pNode->GetNodeAttribute()->GetAttributeType());
        FbxAMatrix lGeometryOffset = GetGeometry(pNode);
        FbxAMatrix lGlobalOffPosition = lGlobalPosition * lGeometryOffset;
        

        switch (lAttributeType) {
            default:
                break;
            case FbxNodeAttribute::eMarker:
                DisplayMarker(pNode);
                break;

            case FbxNodeAttribute::eSkeleton:
                DisplaySkeleton(pNode);
                break;

            case FbxNodeAttribute::eMesh:
                DisplayMesh(pNode, lGlobalOffPosition);
                break;

            case FbxNodeAttribute::eNurbs:
                //DisplayNurb(pNode);
                break;

            case FbxNodeAttribute::ePatch:
                //DisplayPatch(pNode);
                break;

            case FbxNodeAttribute::eCamera:
                //DisplayCamera(pNode);
                break;

            case FbxNodeAttribute::eLight:
                //DisplayLight(pNode);
                break;

            case FbxNodeAttribute::eLODGroup:
                //DisplayLodGroup(pNode);
                break;
        }
    }

    //DisplayUserProperties(pNode);
    //DisplayTarget(pNode);
    //DisplayPivotsAndLimits(pNode);
    //DisplayTransformPropagation(pNode);

	for (i = 0; i < pNode->GetChildCount(); i++) {
		DisplayContent(pNode->GetChild(i), pTime, pAnimLayer, lGlobalPosition, nullptr);
	}
}

void AssetLoader::FbxLoader::DisplayContent(FbxScene* pScene) {
    int i;
    FbxNode* lNode = pScene->GetRootNode();

    //FbxPose* lPose = NULL;
    //if (mPoseIndex != -1) {
    //    lPose = mScene->GetPose(mPoseIndex);
    //}

    // If one node is selected, draw it and its children.
    FbxAMatrix lDummyGlobalPosition;
    FbxTime lDummyTime;

    if (lNode) {
        for (i = 0; i < lNode->GetChildCount(); i++) {
            DisplayContent(lNode->GetChild(i), lDummyTime, nullptr, lDummyGlobalPosition, nullptr);
        }
    }
}


AssetLoader::FbxLoader::FbxLoader() {
    // Prepare the FBX SDK.
    InitializeSdkObjects(lSdkManager, lScene);
    // Load the scene.
}

AssetLoader::FbxLoader::~FbxLoader() {
    // Destroy all objects created by the FBX SDK.
    DestroySdkObjects(lSdkManager, lResult);
}

std::vector<ECS::StaticMesh>& AssetLoader::FbxLoader::LoadAssetFromFile(std::string_view InFileName) {
    // The example can take a FBX file as an argument.
    std::lock_guard<std::mutex> lock(mMeshMutex);
    if (!mStaticMeshes.empty())
    {
        std::vector<ECS::StaticMesh> emptyVector = {};
        mStaticMeshes.swap(emptyVector);
    }

    std::string_view absFileName = InFileName;

    if (!std::filesystem::exists(absFileName))
    {
        absFileName = mModulePath.string() + InFileName.data();
    }

    FbxString lFilePath(absFileName.data());

    if (lFilePath.IsEmpty()) {
        lResult = false;
        FBXSDK_printf("\n\nUsage: ImportScene <FBX file name>\n\n");
        return mStaticMeshes;
    } else {
        FBXSDK_printf("\n\nFile: %s\n\n", lFilePath.Buffer());
        lResult = LoadScene(lSdkManager, lScene, lFilePath.Buffer());
        LoadTextureMaterial(lScene, lFilePath.Buffer());
        // Convert mesh, NURBS and patch into triangle mesh
        FbxGeometryConverter lGeomConverter(lSdkManager);
        Ensures(lGeomConverter.Triangulate(lScene, /*replace*/ true));
        // Convert Axis System to what is used in this example, if needed
        //FbxAxisSystem SceneAxisSystem = lScene->GetGlobalSettings().GetAxisSystem();
        //FbxAxisSystem OurAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityEven, FbxAxisSystem::eLeftHanded);
        //if (SceneAxisSystem != OurAxisSystem) {
        //    OurAxisSystem.ConvertScene(lScene);
        //}

        const FbxSystemUnit::ConversionOptions lConversionOptions = {
            false, /* mConvertRrsNodes */
            true,  /* mConvertLimits */
            true,  /* mConvertClusters */
            true,  /* mConvertLightIntensity */
            true,  /* mConvertPhotometricLProperties */
            true   /* mConvertCameraClipPlanes */
        };

        // Convert the scene to meters using the defined options.
        //FbxSystemUnit::mm.ConvertScene(lScene, lConversionOptions);
    }

    if (lResult == false) {
        FBXSDK_printf("\n\nAn error occurred while loading the scene...");
        return mStaticMeshes;
    } else {
        FBXSDK_printf("\n\nScene Loaded Successfully...");
        // Display the scene.
        DisplayMetaData(lScene);

        //FBXSDK_printf("\n\n---------------------\nGlobal Light Settings\n---------------------\n\n");

        //if (gVerbose)
        //    DisplayGlobalLightSettings(&lScene->GetGlobalSettings());
        //
        //FBXSDK_printf("\n\n----------------------\nGlobal Camera Settings\n----------------------\n\n");
        //
        //if (gVerbose)
        //    DisplayGlobalCameraSettings(&lScene->GetGlobalSettings());
        //
        //FBXSDK_printf("\n\n--------------------\nGlobal Time Settings\n--------------------\n\n");
        //
        //if (gVerbose)
        //    DisplayGlobalTimeSettings(&lScene->GetGlobalSettings());
        //
        //FBXSDK_printf("\n\n---------\nHierarchy\n---------\n\n");
        //
        //if (gVerbose)
        //    DisplayHierarchy(lScene);

        FBXSDK_printf("\n\n------------\nNode Content\n------------\n\n");

        if (gVerbose)
            DisplayContent(lScene);

        //FBXSDK_printf("\n\n----\nPose\n----\n\n");
        //
        //if (gVerbose)
        //    DisplayPose(lScene);
        //
        //FBXSDK_printf("\n\n---------\nAnimation\n---------\n\n");
        //
        //if (gVerbose)
        //    DisplayAnimation(lScene);
        //
        ////now display generic information
        //
        //FBXSDK_printf("\n\n---------\nGeneric Information\n---------\n\n");
        //if (gVerbose)
        //    DisplayGenericInfo(lScene);
        return mStaticMeshes;
    }
}

//AssetLoader::FbxLoader& AssetLoader::FbxLoader::GetInstance() 
//{
//    std::call_once(mCallOnce, []() 
//        {
//            sLoader = new FbxLoader;
//        });
//    return *sLoader;
//}


void DisplayMetaDataConnections(FbxObject* pObject) {
    int nbMetaData = pObject->GetSrcObjectCount<FbxObjectMetaData>();
    if (nbMetaData > 0)
        DisplayString("    MetaData connections ");

    for (int i = 0; i < nbMetaData; i++) {
        FbxObjectMetaData* metaData = pObject->GetSrcObject<FbxObjectMetaData>(i);
        DisplayString("        Name: ", (char*)metaData->GetName());
    }
}

void DisplayString(const char* pHeader, const char* pValue /* = "" */, const char* pSuffix /* = "" */) {
    FbxString lString;

    lString = pHeader;
    lString += pValue;
    lString += pSuffix;
    lString += "\n";
    FBXSDK_printf(lString);
}


void DisplayBool(const char* pHeader, bool pValue, const char* pSuffix /* = "" */) {
    FbxString lString;

    lString = pHeader;
    lString += pValue ? "true" : "false";
    lString += pSuffix;
    lString += "\n";
    FBXSDK_printf(lString);
}


void DisplayInt(const char* pHeader, int pValue, const char* pSuffix /* = "" */) {
    FbxString lString;

    lString = pHeader;
    lString += pValue;
    lString += pSuffix;
    lString += "\n";
    FBXSDK_printf(lString);
}


void DisplayDouble(const char* pHeader, double pValue, const char* pSuffix /* = "" */) {
    FbxString lString;
    FbxString lFloatValue = (float)pValue;

    lFloatValue = pValue <= -HUGE_VAL ? "-INFINITY" : lFloatValue.Buffer();
    lFloatValue = pValue >= HUGE_VAL ? "INFINITY" : lFloatValue.Buffer();

    lString = pHeader;
    lString += lFloatValue;
    lString += pSuffix;
    lString += "\n";
    FBXSDK_printf(lString);
}


void Display2DVector(const char* pHeader, FbxVector2 pValue, const char* pSuffix /* = "" */) {
    FbxString lString;
    FbxString lFloatValue1 = (float)pValue[0];
    FbxString lFloatValue2 = (float)pValue[1];

    lFloatValue1 = pValue[0] <= -HUGE_VAL ? "-INFINITY" : lFloatValue1.Buffer();
    lFloatValue1 = pValue[0] >= HUGE_VAL ? "INFINITY" : lFloatValue1.Buffer();
    lFloatValue2 = pValue[1] <= -HUGE_VAL ? "-INFINITY" : lFloatValue2.Buffer();
    lFloatValue2 = pValue[1] >= HUGE_VAL ? "INFINITY" : lFloatValue2.Buffer();

    lString = pHeader;
    lString += lFloatValue1;
    lString += ", ";
    lString += lFloatValue2;
    lString += pSuffix;
    lString += "\n";
    FBXSDK_printf(lString);
}


void Display3DVector(const char* pHeader, FbxVector4 pValue, const char* pSuffix /* = "" */) {
    FbxString lString;
    FbxString lFloatValue1 = (float)pValue[0];
    FbxString lFloatValue2 = (float)pValue[1];
    FbxString lFloatValue3 = (float)pValue[2];

    lFloatValue1 = pValue[0] <= -HUGE_VAL ? "-INFINITY" : lFloatValue1.Buffer();
    lFloatValue1 = pValue[0] >= HUGE_VAL ? "INFINITY" : lFloatValue1.Buffer();
    lFloatValue2 = pValue[1] <= -HUGE_VAL ? "-INFINITY" : lFloatValue2.Buffer();
    lFloatValue2 = pValue[1] >= HUGE_VAL ? "INFINITY" : lFloatValue2.Buffer();
    lFloatValue3 = pValue[2] <= -HUGE_VAL ? "-INFINITY" : lFloatValue3.Buffer();
    lFloatValue3 = pValue[2] >= HUGE_VAL ? "INFINITY" : lFloatValue3.Buffer();

    lString = pHeader;
    lString += lFloatValue1;
    lString += ", ";
    lString += lFloatValue2;
    lString += ", ";
    lString += lFloatValue3;
    lString += pSuffix;
    lString += "\n";
    FBXSDK_printf(lString);
}

void Display4DVector(const char* pHeader, FbxVector4 pValue, const char* pSuffix /* = "" */) {
    FbxString lString;
    FbxString lFloatValue1 = (float)pValue[0];
    FbxString lFloatValue2 = (float)pValue[1];
    FbxString lFloatValue3 = (float)pValue[2];
    FbxString lFloatValue4 = (float)pValue[3];

    lFloatValue1 = pValue[0] <= -HUGE_VAL ? "-INFINITY" : lFloatValue1.Buffer();
    lFloatValue1 = pValue[0] >= HUGE_VAL ? "INFINITY" : lFloatValue1.Buffer();
    lFloatValue2 = pValue[1] <= -HUGE_VAL ? "-INFINITY" : lFloatValue2.Buffer();
    lFloatValue2 = pValue[1] >= HUGE_VAL ? "INFINITY" : lFloatValue2.Buffer();
    lFloatValue3 = pValue[2] <= -HUGE_VAL ? "-INFINITY" : lFloatValue3.Buffer();
    lFloatValue3 = pValue[2] >= HUGE_VAL ? "INFINITY" : lFloatValue3.Buffer();
    lFloatValue4 = pValue[3] <= -HUGE_VAL ? "-INFINITY" : lFloatValue4.Buffer();
    lFloatValue4 = pValue[3] >= HUGE_VAL ? "INFINITY" : lFloatValue4.Buffer();

    lString = pHeader;
    lString += lFloatValue1;
    lString += ", ";
    lString += lFloatValue2;
    lString += ", ";
    lString += lFloatValue3;
    lString += ", ";
    lString += lFloatValue4;
    lString += pSuffix;
    lString += "\n";
    FBXSDK_printf(lString);
}


void DisplayColor(const char* pHeader, FbxPropertyT<FbxDouble3> pValue, const char* pSuffix /* = "" */)

{
    FbxString lString;

    lString = pHeader;
    //lString += (float) pValue.mRed;
    //lString += (double)pValue.GetArrayItem(0);
    lString += " (red), ";
    //lString += (float) pValue.mGreen;
    //lString += (double)pValue.GetArrayItem(1);
    lString += " (green), ";
    //lString += (float) pValue.mBlue;
    //lString += (double)pValue.GetArrayItem(2);
    lString += " (blue)";
    lString += pSuffix;
    lString += "\n";
    FBXSDK_printf(lString);
}


void DisplayColor(const char* pHeader, FbxColor pValue, const char* pSuffix /* = "" */) {
    FbxString lString;

    lString = pHeader;
    lString += (float)pValue.mRed;

    lString += " (red), ";
    lString += (float)pValue.mGreen;

    lString += " (green), ";
    lString += (float)pValue.mBlue;

    lString += " (blue)";
    lString += pSuffix;
    lString += "\n";
    FBXSDK_printf(lString);
}

void DisplayControlsPoints(FbxMesh* pMesh) {
    int i, lControlPointsCount = pMesh->GetControlPointsCount();
    FbxVector4* lControlPoints = pMesh->GetControlPoints();

    DisplayString("    Control Points");

    for (i = 0; i < lControlPointsCount; i++) {
        DisplayInt("        Control Point ", i);
        Display3DVector("            Coordinates: ", lControlPoints[i]);

        for (int j = 0; j < pMesh->GetElementNormalCount(); j++) {
            FbxGeometryElementNormal* leNormals = pMesh->GetElementNormal(j);
            if (leNormals->GetMappingMode() == FbxGeometryElement::eByControlPoint) {
                char header[100];
                FBXSDK_sprintf(header, 100, "            Normal Vector: ");
                if (leNormals->GetReferenceMode() == FbxGeometryElement::eDirect)
                    Display3DVector(header, leNormals->GetDirectArray().GetAt(i));
            }
        }
    }

    DisplayString("");
}


void AssetLoader::FbxLoader::DisplayPolygons(FbxMesh* pMesh) {
    int i, j, lPolygonCount = pMesh->GetPolygonCount();
    FbxVector4* lControlPoints = pMesh->GetControlPoints();
    char header[100];

    DisplayString("    Polygons");

    int vertexId = 0;
    for (i = 0; i < lPolygonCount; i++) {
        DisplayInt("        Polygon ", i);
        int l;

        for (l = 0; l < pMesh->GetElementPolygonGroupCount(); l++) {
            FbxGeometryElementPolygonGroup* lePolgrp = pMesh->GetElementPolygonGroup(l);
            switch (lePolgrp->GetMappingMode()) {
                case FbxGeometryElement::eByPolygon:
                    if (lePolgrp->GetReferenceMode() == FbxGeometryElement::eIndex) {
                        FBXSDK_sprintf(header, 100, "        Assigned to group: ");
                        int polyGroupId = lePolgrp->GetIndexArray().GetAt(i);
                        DisplayInt(header, polyGroupId);
                        break;
                    }
                default:
                    // any other mapping modes don't make sense
                    DisplayString("        \"unsupported group assignment\"");
                    break;
            }
        }

        int lPolygonSize = pMesh->GetPolygonSize(i);

        for (j = 0; j < lPolygonSize; j++) {
            int lControlPointIndex = pMesh->GetPolygonVertex(i, j);
            if (lControlPointIndex < 0) {
                DisplayString("            Coordinates: Invalid index found!");
                continue;
            } else
                Display3DVector("            Coordinates: ", lControlPoints[lControlPointIndex]);

            for (l = 0; l < pMesh->GetElementVertexColorCount(); l++) {
                FbxGeometryElementVertexColor* leVtxc = pMesh->GetElementVertexColor(l);
                FBXSDK_sprintf(header, 100, "            Color vertex: ");

                switch (leVtxc->GetMappingMode()) {
                    default:
                        break;
                    case FbxGeometryElement::eByControlPoint:
                        switch (leVtxc->GetReferenceMode()) {
                            case FbxGeometryElement::eDirect:
                                DisplayColor(header, leVtxc->GetDirectArray().GetAt(lControlPointIndex));
                                break;
                            case FbxGeometryElement::eIndexToDirect: {
                                int id = leVtxc->GetIndexArray().GetAt(lControlPointIndex);
                                DisplayColor(header, leVtxc->GetDirectArray().GetAt(id));
                            } break;
                            default:
                                break;  // other reference modes not shown here!
                        }
                        break;

                    case FbxGeometryElement::eByPolygonVertex: {
                        switch (leVtxc->GetReferenceMode()) {
                            case FbxGeometryElement::eDirect:
                                DisplayColor(header, leVtxc->GetDirectArray().GetAt(vertexId));
                                break;
                            case FbxGeometryElement::eIndexToDirect: {
                                int id = leVtxc->GetIndexArray().GetAt(vertexId);
                                DisplayColor(header, leVtxc->GetDirectArray().GetAt(id));
                            } break;
                            default:
                                break;  // other reference modes not shown here!
                        }
                    } break;

                    case FbxGeometryElement::eByPolygon:  // doesn't make much sense for UVs
                    case FbxGeometryElement::eAllSame:    // doesn't make much sense for UVs
                    case FbxGeometryElement::eNone:       // doesn't make much sense for UVs
                        break;
                }
            }
            for (l = 0; l < pMesh->GetElementUVCount(); ++l) {
                FbxGeometryElementUV* leUV = pMesh->GetElementUV(l);
                FBXSDK_sprintf(header, 100, "            Texture UV: ");

                switch (leUV->GetMappingMode()) {
                    default:
                        break;
                    case FbxGeometryElement::eByControlPoint:
                        switch (leUV->GetReferenceMode()) {
                            case FbxGeometryElement::eDirect:
                                Display2DVector(header, leUV->GetDirectArray().GetAt(lControlPointIndex));
                                break;
                            case FbxGeometryElement::eIndexToDirect: {
                                int id = leUV->GetIndexArray().GetAt(lControlPointIndex);
                                Display2DVector(header, leUV->GetDirectArray().GetAt(id));
                            } break;
                            default:
                                break;  // other reference modes not shown here!
                        }
                        break;

                    case FbxGeometryElement::eByPolygonVertex: {
                        int lTextureUVIndex = pMesh->GetTextureUVIndex(i, j);
                        switch (leUV->GetReferenceMode()) {
                            case FbxGeometryElement::eDirect:
                            case FbxGeometryElement::eIndexToDirect: {
                                Display2DVector(header, leUV->GetDirectArray().GetAt(lTextureUVIndex));
                            } break;
                            default:
                                break;  // other reference modes not shown here!
                        }
                    } break;

                    case FbxGeometryElement::eByPolygon:  // doesn't make much sense for UVs
                    case FbxGeometryElement::eAllSame:    // doesn't make much sense for UVs
                    case FbxGeometryElement::eNone:       // doesn't make much sense for UVs
                        break;
                }
            }
            for (l = 0; l < pMesh->GetElementNormalCount(); ++l) {
                FbxGeometryElementNormal* leNormal = pMesh->GetElementNormal(l);
                FBXSDK_sprintf(header, 100, "            Normal: ");

                if (leNormal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex) {
                    switch (leNormal->GetReferenceMode()) {
                        case FbxGeometryElement::eDirect:
                            Display3DVector(header, leNormal->GetDirectArray().GetAt(vertexId));
                            break;
                        case FbxGeometryElement::eIndexToDirect: {
                            int id = leNormal->GetIndexArray().GetAt(vertexId);
                            Display3DVector(header, leNormal->GetDirectArray().GetAt(id));
                        } break;
                        default:
                            break;  // other reference modes not shown here!
                    }
                }
            }
            for (l = 0; l < pMesh->GetElementTangentCount(); ++l) {
                FbxGeometryElementTangent* leTangent = pMesh->GetElementTangent(l);
                FBXSDK_sprintf(header, 100, "            Tangent: ");

                if (leTangent->GetMappingMode() == FbxGeometryElement::eByPolygonVertex) {
                    switch (leTangent->GetReferenceMode()) {
                        case FbxGeometryElement::eDirect:
                            Display3DVector(header, leTangent->GetDirectArray().GetAt(vertexId));
                            break;
                        case FbxGeometryElement::eIndexToDirect: {
                            int id = leTangent->GetIndexArray().GetAt(vertexId);
                            Display3DVector(header, leTangent->GetDirectArray().GetAt(id));
                        } break;
                        default:
                            break;  // other reference modes not shown here!
                    }
                }
            }
            for (l = 0; l < pMesh->GetElementBinormalCount(); ++l) {

                FbxGeometryElementBinormal* leBinormal = pMesh->GetElementBinormal(l);

                FBXSDK_sprintf(header, 100, "            Binormal: ");
                if (leBinormal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex) {
                    switch (leBinormal->GetReferenceMode()) {
                        case FbxGeometryElement::eDirect:
                            Display3DVector(header, leBinormal->GetDirectArray().GetAt(vertexId));
                            break;
                        case FbxGeometryElement::eIndexToDirect: {
                            int id = leBinormal->GetIndexArray().GetAt(vertexId);
                            Display3DVector(header, leBinormal->GetDirectArray().GetAt(id));
                        } break;
                        default:
                            break;  // other reference modes not shown here!
                    }
                }
            }
            vertexId++;
        }  // for polygonSize
    }      // for polygonCount


    //check visibility for the edges of the mesh
    for (int l = 0; l < pMesh->GetElementVisibilityCount(); ++l) {
        FbxGeometryElementVisibility* leVisibility = pMesh->GetElementVisibility(l);
        FBXSDK_sprintf(header, 100, "    Edge Visibility : ");
        DisplayString(header);
        switch (leVisibility->GetMappingMode()) {
            default:
                break;
                //should be eByEdge
            case FbxGeometryElement::eByEdge:
                //should be eDirect
                for (j = 0; j != pMesh->GetMeshEdgeCount(); ++j) {
                    DisplayInt("        Edge ", j);
                    DisplayBool("              Edge visibility: ", leVisibility->GetDirectArray().GetAt(j));
                }

                break;
        }
    }
    DisplayString("");
}

bool AssetLoader::FbxLoader::LoadStaticMesh(FbxMesh* pMesh) {

    if (!pMesh->GetNode())
        return false;
    ECS::StaticMesh newMesh;
    const int lPolygonCount = pMesh->GetPolygonCount();
    newMesh.mName = pMesh->GetName();
    // Count the polygon count of each material
    FbxLayerElementArrayTemplate<int>* lMaterialIndice = NULL;
    FbxGeometryElement::EMappingMode lMaterialMappingMode = FbxGeometryElement::eNone;
    if (pMesh->GetElementMaterial()) {
        lMaterialIndice = &pMesh->GetElementMaterial()->GetIndexArray();
        lMaterialMappingMode = pMesh->GetElementMaterial()->GetMappingMode();
        if (lMaterialIndice && lMaterialMappingMode == FbxGeometryElement::eByPolygon) {
            FBX_ASSERT(lMaterialIndice->GetCount() == lPolygonCount);
            if (lMaterialIndice->GetCount() == lPolygonCount) {
                // Count the faces of each material
                for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex) {
                    const int lMaterialIndex = lMaterialIndice->GetAt(lPolygonIndex);
                    if (newMesh.mSubmeshMap.find(lMaterialIndex) == newMesh.mSubmeshMap.end())
                    {
                        newMesh.mSubmeshMap[lMaterialIndex] = {};
                    }
                    newMesh.mSubmeshMap[lMaterialIndex].TriangleCount += 1;
                }

                // Make sure we have no "holes" (NULL) in the mSubMeshes table. This can happen
                // if, in the loop above, we resized the mSubMeshes by more than one slot.
                //for (int i = 0; i < newMesh.mSubMeshes.size(); i++) {
                //    if (newMesh.mSubMeshes[i] == NULL)
                //        newMesh.mSubMeshes[i] = new ECS::SubMesh;
                //}

                // Record the offset (how many vertex)
                const int lMaterialCount = newMesh.mSubmeshMap.size();
                int lOffset = 0;

                for (auto& submesh : newMesh.mSubmeshMap)
                {
                    submesh.second.IndexOffset = lOffset;
                    lOffset += submesh.second.TriangleCount * 3;
                    // This will be used as counter in the following procedures, reset to
                    submesh.second.IndexCount = submesh.second.TriangleCount * 3;
                    submesh.second.TriangleCount = 0;
                }
                FBX_ASSERT(lOffset == lPolygonCount * 3);
            }
        }
    }

    // All faces will use the same material.
    if (newMesh.mSubmeshMap.size() == 0) {
        newMesh.mSubmeshMap[0] = {};
    }

    // Congregate all the data of a mesh to be cached in VBOs.
    // If normal or UV is by polygon vertex, record all vertex attributes by polygon vertex.
    newMesh.mHasNormal = pMesh->GetElementNormalCount() > 0;
    newMesh.mHasUV = pMesh->GetElementUVCount() > 0;
    FbxGeometryElement::EMappingMode lNormalMappingMode = FbxGeometryElement::eNone;
    FbxGeometryElement::EMappingMode lUVMappingMode = FbxGeometryElement::eNone;
    if (newMesh.mHasNormal) {
        lNormalMappingMode = pMesh->GetElementNormal(0)->GetMappingMode();
        if (lNormalMappingMode == FbxGeometryElement::eNone) {
            newMesh.mHasNormal = false;
        }
        if (newMesh.mHasNormal && lNormalMappingMode != FbxGeometryElement::eByControlPoint) {
            newMesh.mAllByControlPoint = false;
        }
    }
    if (newMesh.mHasUV) {
        lUVMappingMode = pMesh->GetElementUV(0)->GetMappingMode();
        if (lUVMappingMode == FbxGeometryElement::eNone) {
            newMesh.mHasUV = false;
        }
        if (newMesh.mHasUV && lUVMappingMode != FbxGeometryElement::eByControlPoint) {
            newMesh.mAllByControlPoint = false;
        }
    }

    // Allocate the array memory, by control point or by polygon vertex.
    int lPolygonVertexCount = pMesh->GetControlPointsCount();
    if (!newMesh.mAllByControlPoint) {
        lPolygonVertexCount = lPolygonCount * TRIANGLE_VERTEX_COUNT;
    }
    float* lVertices = new float[lPolygonVertexCount * POSITION_STRIDE];
    //unsigned int* lIndices = new unsigned int[lPolygonCount * TRIANGLE_VERTEX_COUNT];
    newMesh.mIndices.resize(lPolygonCount * TRIANGLE_VERTEX_COUNT);
    newMesh.mVertices.resize(lPolygonVertexCount);
    float* lNormals = NULL;
    if (newMesh.mHasNormal) {
        lNormals = new float[lPolygonVertexCount * NORMAL_STRIDE];
    }
    float* lUVs = NULL;
    FbxStringList lUVNames;
    pMesh->GetUVSetNames(lUVNames);
    const char* lUVName = NULL;
    if (newMesh.mHasUV && lUVNames.GetCount()) {
        lUVs = new float[lPolygonVertexCount * UV_STRIDE];
        lUVName = lUVNames[0];
    }

    // Populate the array with vertex attribute, if by control point.
    const FbxVector4* lControlPoints = pMesh->GetControlPoints();
    FbxVector4 lCurrentVertex;
    FbxVector4 lCurrentNormal;
    FbxVector2 lCurrentUV;
    if (newMesh.mAllByControlPoint) {
        const FbxGeometryElementNormal* lNormalElement = NULL;
        const FbxGeometryElementUV* lUVElement = NULL;
        if (newMesh.mHasNormal) {
            lNormalElement = pMesh->GetElementNormal(0);
        }
        if (newMesh.mHasUV) {
            lUVElement = pMesh->GetElementUV(0);
        }
        for (int lIndex = 0; lIndex < lPolygonVertexCount; ++lIndex) {
            // Save the vertex position.
            lCurrentVertex = lControlPoints[lIndex];
            newMesh.mVertices[lIndex].pos[0] = static_cast<float>(lCurrentVertex[0]);
            newMesh.mVertices[lIndex].pos[1] = static_cast<float>(lCurrentVertex[1]);
            newMesh.mVertices[lIndex].pos[2] = static_cast<float>(lCurrentVertex[2]);
            newMesh.mVertices[lIndex].pos[3] = 1;

            // Save the normal.
            if (newMesh.mHasNormal) {
                int lNormalIndex = lIndex;
                if (lNormalElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect) {
                    lNormalIndex = lNormalElement->GetIndexArray().GetAt(lIndex);
                }
                lCurrentNormal = lNormalElement->GetDirectArray().GetAt(lNormalIndex);
                newMesh.mVertices[lIndex].normal[0] = static_cast<float>(lCurrentNormal[0]);
                newMesh.mVertices[lIndex].normal[1] = static_cast<float>(lCurrentNormal[1]);
                newMesh.mVertices[lIndex].normal[2] = static_cast<float>(lCurrentNormal[2]);
            }

            // Save the UV.
            if (newMesh.mHasUV) {
                int lUVIndex = lIndex;
                if (lUVElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect) {
                    lUVIndex = lUVElement->GetIndexArray().GetAt(lIndex);
                }
                lCurrentUV = lUVElement->GetDirectArray().GetAt(lUVIndex);
                newMesh.mVertices[lIndex].textureCoord[0] = static_cast<float>(lCurrentUV[0]);
                newMesh.mVertices[lIndex].textureCoord[1] = static_cast<float>(lCurrentUV[1]);
            }
        }
    }

    int lVertexCount = 0;
    for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex) {
        // The material for current face.
        int lMaterialIndex = 0;
        if (lMaterialIndice && lMaterialMappingMode == FbxGeometryElement::eByPolygon) {
            lMaterialIndex = lMaterialIndice->GetAt(lPolygonIndex);
        }

        // Where should I save the vertex attribute index, according to the material
        const int lIndexOffset = newMesh.mSubmeshMap[lMaterialIndex].IndexOffset
                                 + newMesh.mSubmeshMap[lMaterialIndex].TriangleCount * 3;
        for (int lVerticeIndex = 0; lVerticeIndex < TRIANGLE_VERTEX_COUNT; ++lVerticeIndex) {
            const int lControlPointIndex = pMesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);
            // If the lControlPointIndex is -1, we probably have a corrupted mesh data. At this point,
            // it is not guaranteed that the cache will work as expected.
            if (lControlPointIndex >= 0) {
                if (newMesh.mAllByControlPoint) {
                    newMesh.mIndices[lIndexOffset + lVerticeIndex] = static_cast<unsigned int>(lControlPointIndex);
                }
                // Populate the array with vertex attribute, if by polygon vertex.
                else {
                    newMesh.mIndices[lIndexOffset + lVerticeIndex] = static_cast<unsigned int>(lVertexCount);

                    lCurrentVertex = lControlPoints[lControlPointIndex];
                    newMesh.mVertices[lVertexCount].pos[0] = static_cast<float>(lCurrentVertex[0]);
                    newMesh.mVertices[lVertexCount].pos[1] = static_cast<float>(lCurrentVertex[1]);
                    newMesh.mVertices[lVertexCount].pos[2] = static_cast<float>(lCurrentVertex[2]);
                    newMesh.mVertices[lVertexCount].pos[3] = 1;
                    if (newMesh.mHasNormal) {
                        pMesh->GetPolygonVertexNormal(lPolygonIndex, lVerticeIndex, lCurrentNormal);
                        newMesh.mVertices[lVertexCount].normal[0] = static_cast<float>(lCurrentNormal[0]);
                        newMesh.mVertices[lVertexCount].normal[1] = static_cast<float>(lCurrentNormal[1]);
                        newMesh.mVertices[lVertexCount].normal[2] = static_cast<float>(lCurrentNormal[2]);
                    }

                    if (newMesh.mHasUV) {
                        bool lUnmappedUV;
                        pMesh->GetPolygonVertexUV(lPolygonIndex, lVerticeIndex, lUVName, lCurrentUV, lUnmappedUV);
                        newMesh.mVertices[lVertexCount].textureCoord[0] = static_cast<float>(lCurrentUV[0]);
                        newMesh.mVertices[lVertexCount].textureCoord[1] = static_cast<float>(lCurrentUV[1]);
                    }
                }
            }
            ++lVertexCount;
        }
        newMesh.mSubmeshMap[lMaterialIndex].TriangleCount += 1;
    }
	DisplayMaterialConnections(pMesh,newMesh);
	DisplayMaterial(pMesh,newMesh);
    mStaticMeshes.push_back(newMesh);
    return true;
}

void DisplayTextureNames(FbxProperty& pProperty, FbxString& pConnectionString) {
    int lLayeredTextureCount = pProperty.GetSrcObjectCount<FbxLayeredTexture>();
    if (lLayeredTextureCount > 0) {
        for (int j = 0; j < lLayeredTextureCount; ++j) {
            FbxLayeredTexture* lLayeredTexture = pProperty.GetSrcObject<FbxLayeredTexture>(j);
            int lNbTextures = lLayeredTexture->GetSrcObjectCount<FbxTexture>();
            //pConnectionString += " Texture ";

            for (int k = 0; k < lNbTextures; ++k) {
                //lConnectionString += k;
                //pConnectionString += "\"";
                pConnectionString += (char*)lLayeredTexture->GetName();
                //pConnectionString += "\"";
                //pConnectionString += " ";
            }
            //pConnectionString += "of ";
            //pConnectionString += pProperty.GetName();
            //pConnectionString += " on layer ";
            //pConnectionString += j;
        }
        //pConnectionString += " |";
    } else {
        //no layered texture simply get on the property
        int lNbTextures = pProperty.GetSrcObjectCount<FbxTexture>();

        if (lNbTextures > 0) {
            //pConnectionString += " Texture ";
            //pConnectionString += " ";

            for (int j = 0; j < lNbTextures; ++j) {
                FbxTexture* lTexture = pProperty.GetSrcObject<FbxTexture>(j);
                if (lTexture) {
                    //pConnectionString += "\"";
                    pConnectionString += (char*)lTexture->GetName();
                    //pConnectionString += "\"";
                    //pConnectionString += " ";
                }
            }
            //pConnectionString += "of ";
            //pConnectionString += pProperty.GetName();
            //pConnectionString += " |";
        }
    }
}

void DisplayMaterialTextureConnections(FbxSurfaceMaterial* pMaterial, ECS::StaticMesh& InMesh, char* header, int pMatId, int l) {
    if (!pMaterial)
        return;

    FbxString lConnectionString = "            Material %d -- ";
    //Show all the textures

    FbxProperty lProperty;
    //Diffuse Textures
    FbxString diffuseColorName;
    lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sDiffuse);
    DisplayTextureNames(lProperty, diffuseColorName);
    InMesh.mMatTextureName[pMatId] = diffuseColorName;

    //DiffuseFactor Textures
    //lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sDiffuseFactor);
    //DisplayTextureNames(lProperty, lConnectionString);
    //
    ////Emissive Textures
    //lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sEmissive);
    //DisplayTextureNames(lProperty, lConnectionString);
    //
    ////EmissiveFactor Textures
    //lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sEmissiveFactor);
    //DisplayTextureNames(lProperty, lConnectionString);
    //
    //
    ////Ambient Textures
    //lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sAmbient);
    //DisplayTextureNames(lProperty, lConnectionString);
    //
    ////AmbientFactor Textures
    //lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sAmbientFactor);
    //DisplayTextureNames(lProperty, lConnectionString);
    //
    ////Specular Textures
    //lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sSpecular);
    //DisplayTextureNames(lProperty, lConnectionString);
    //
    ////SpecularFactor Textures
    //lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sSpecularFactor);
    //DisplayTextureNames(lProperty, lConnectionString);
    //
    ////Shininess Textures
    //lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sShininess);
    //DisplayTextureNames(lProperty, lConnectionString);
    //
    ////Bump Textures
    //lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sBump);
    //DisplayTextureNames(lProperty, lConnectionString);
    //
    ////Normal Map Textures
    //lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sNormalMap);
    //DisplayTextureNames(lProperty, lConnectionString);
    //
    ////Transparent Textures
    //lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sTransparentColor);
    //DisplayTextureNames(lProperty, lConnectionString);
    //
    ////TransparencyFactor Textures
    //lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sTransparencyFactor);
    //DisplayTextureNames(lProperty, lConnectionString);
    //
    ////Reflection Textures
    //lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sReflection);
    //DisplayTextureNames(lProperty, lConnectionString);
    //
    ////ReflectionFactor Textures
    //lProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sReflectionFactor);
    //DisplayTextureNames(lProperty, lConnectionString);
    //
    ////Update header with material info
    //bool lStringOverflow = (lConnectionString.GetLen() + 10
    //                        >= MAT_HEADER_LENGTH);  // allow for string length and some padding for "%d"
    //if (lStringOverflow) {
    //    // Truncate string!
    //    lConnectionString = lConnectionString.Left(MAT_HEADER_LENGTH - 10);
    //    lConnectionString = lConnectionString + "...";
    //}
    //FBXSDK_sprintf(header, MAT_HEADER_LENGTH, lConnectionString.Buffer(), pMatId, l);
    //DisplayString(header);
}

void DisplayMaterialConnections(const FbxMesh* pMesh,ECS::StaticMesh& InMesh) {
    int i, l, lPolygonCount = pMesh->GetPolygonCount();

    char header[MAT_HEADER_LENGTH];

     DisplayString("    Polygons Material Connections");

    //check whether the material maps with only one mesh
    bool lIsAllSame = true;
    for (l = 0; l < pMesh->GetElementMaterialCount(); l++) {

        const FbxGeometryElementMaterial* lMaterialElement = pMesh->GetElementMaterial(l);
        if (lMaterialElement->GetMappingMode() == FbxGeometryElement::eByPolygon) {
            lIsAllSame = false;
            break;
        }
    }
	DisplayInt("        Material Count ", pMesh->GetElementMaterialCount());

    //For eAllSame mapping type, just out the material and texture mapping info once
    if (lIsAllSame) {
        for (l = 0; l < pMesh->GetElementMaterialCount(); l++) {

            const FbxGeometryElementMaterial* lMaterialElement = pMesh->GetElementMaterial(l);
            if (lMaterialElement->GetMappingMode() == FbxGeometryElement::eAllSame) {
                FbxSurfaceMaterial* lMaterial =
                        pMesh->GetNode()->GetMaterial(lMaterialElement->GetIndexArray().GetAt(0));
                int lMatId = lMaterialElement->GetIndexArray().GetAt(0);
                if (lMatId >= 0) {
                    DisplayInt("        All polygons share the same material in mesh ", l);
                    DisplayMaterialTextureConnections(lMaterial,InMesh, header, lMatId, l);
                }
            }
        }

        //no material
        if (l == 0)
            DisplayString("        no material applied");
    }

    //For eByPolygon mapping type, just out the material and texture mapping info once
    else {
        for (i = 0; i < lPolygonCount; i++) {
            //DisplayInt("        Polygon ", i);

            for (l = 0; l < pMesh->GetElementMaterialCount(); l++) {

                const FbxGeometryElementMaterial* lMaterialElement = pMesh->GetElementMaterial(l);
                FbxSurfaceMaterial* lMaterial = NULL;
                int lMatId = -1;
                lMaterial = pMesh->GetNode()->GetMaterial(lMaterialElement->GetIndexArray().GetAt(i));
                lMatId = lMaterialElement->GetIndexArray().GetAt(i);

                if (lMatId >= 0) {
                    DisplayMaterialTextureConnections(lMaterial,InMesh, header, lMatId, l);
                }
            }
        }
    }
}


void DisplayMaterialMapping(FbxMesh* pMesh) {
    const char* lMappingTypes[] = {
        "None", "By Control Point", "By Polygon Vertex", "By Polygon", "By Edge", "All Same"
    };
    const char* lReferenceMode[] = { "Direct", "Index", "Index to Direct" };

    int lMtrlCount = 0;
    FbxNode* lNode = NULL;
    if (pMesh) {
        lNode = pMesh->GetNode();
        if (lNode)
            lMtrlCount = lNode->GetMaterialCount();
    }

    for (int l = 0; l < pMesh->GetElementMaterialCount(); l++) {
        FbxGeometryElementMaterial* leMat = pMesh->GetElementMaterial(l);
        if (leMat) {
            char header[100];
            FBXSDK_sprintf(header, 100, "    Material Element %d: ", l);
            DisplayString(header);


            DisplayString("           Mapping: ", lMappingTypes[leMat->GetMappingMode()]);
            DisplayString("           ReferenceMode: ", lReferenceMode[leMat->GetReferenceMode()]);

            int lMaterialCount = 0;
            FbxString lString;

            if (leMat->GetReferenceMode() == FbxGeometryElement::eDirect
                || leMat->GetReferenceMode() == FbxGeometryElement::eIndexToDirect) {
                lMaterialCount = lMtrlCount;
            }

            if (leMat->GetReferenceMode() == FbxGeometryElement::eIndex
                || leMat->GetReferenceMode() == FbxGeometryElement::eIndexToDirect) {
                int i;

                lString = "           Indices: ";

                int lIndexArrayCount = leMat->GetIndexArray().GetCount();
                for (i = 0; i < lIndexArrayCount; i++) {
                    lString += leMat->GetIndexArray().GetAt(i);

                    if (i < lIndexArrayCount - 1) {
                        lString += ", ";
                    }
                }

                lString += "\n";

                FBXSDK_printf(lString);
            }
        }
    }

    DisplayString("");
}
