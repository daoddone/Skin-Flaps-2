#ifndef __SURGICALACTIONS_H__
#define __SURGICALACTIONS_H__

#include <string>
#include <vector>
#include <list>
#include "hooks.h"
#include "sutures.h"
#include "surgGraphics.h"
#include "fence.h"

#include "deepCut.h"
#include "skinCutUndermineTets.h"  // replace with above later

#include "json.h"
#include <Vec3f.h>
#include "bccTetScene.h"

// forward declarations
class FacialFlapsGui;
class gl3wGraphics;

class surgicalActions
{
public:
	void sendUserMessage(const char *message, const char *title, bool closeProgram = false);
	bool rightMouseDown(std::string objectHit, float(&position)[3], int triangle);
	bool rightMouseUp(std::string objectHit, float(&position)[3], int triangle);
	bool mouseMotion(float dScreenX, float dScreenY);
	void onKeyDown(int key);
	void onKeyUp(int key);
	inline void setToolState(int toolState){ _bts.setPhysicsPause(toolState < 1 ? false : true); _toolState = toolState; }
	inline int getToolState() { return _toolState; }
	inline void setGl3wGraphics(gl3wGraphics *gl3w) { _gl3w = gl3w; _bts.setGl3wGraphics(gl3w); }
	void setFacialFlapsGui(FacialFlapsGui *ffg) { _ffg = ffg; }
	inline hooks* getHooks() { return &_hooks; }
	inline sutures* getSutures() { return &_sutures; }
	bool loadScene(const char *modelDirectory, const char *sceneFilename);
	inline bccTetScene* getBccTetScene() { return &_bts; }
	inline surgGraphics* getSurgGraphics() { return &_sg; }

	//	inline deepCut* getDeepCutPtr() { return &_incisions; }
	inline skinCutUndermineTets* getDeepCutPtr() { return &_incisions; }  // COURT fix when deepCut added back

	bool loadHistory(const char *historyDir, const char *historyFile);
	void nextHistoryAction();
	bool historyEmpty()	{return _historyArray.size()<1;}
	bool setHistoryAttachPoint(const int triangle, const float(&uv)[2], int &material, float(&historyTexture)[2], Vec3f &historyVec);
	// Input an attach point in current environment. Outputs a material, texture, and displacement for storage in a history file.
	bool getHistoryAttachPoint(const int material, const float(&historyTexture)[2], const Vec3f &displacement, int &triangle, float(&uv)[2], bool findEdge);
	// Input a history attach point from history file. Outputs closest triangle, and parametric uv coord in current environment.
	bool saveSurgicalHistory(const char *fullFilePath);
	const char* getModelDirectory() { return _sceneDir.c_str(); }
	const char* getHistoryDirectory() { return _historyDir.c_str(); }
	void setModelDirectory(const char* sceneDir) { _sceneDir.assign(sceneDir); }
	void setHistoryDirectory(const char* histDir) { _historyDir.assign(histDir); }
	bool saveCurrentObj(const char* fullFilePath, const char* fileNamePrefix);
	void promoteFakeSutures();
	void pausePhysics();
	bool _strongHooks;  // COURT - hack for collision cheating purposes
	std::atomic<bool> physicsDone, newTopology, taskThreadError;
	std::string taskThreadErrorStr;
	bccTetScene _bts;

	surgicalActions();
	~surgicalActions();

private:
    struct float3{	float v[3]; };
	int _toolState;
	gl3wGraphics *_gl3w;
	FacialFlapsGui *_ffg;
	std::vector<int> _pXToPbTetVertices;
	int _originalTriangleNumber;
	int _dragVertex;
	float _dragXyz[3];
	std::string _selectedSurgObject,_dragTissue;
	surgGraphics _sg;	// dynamic triangulated skin object
	hooks _hooks;
	sutures _sutures;
	deepCut _incisions;  // derived from skinCutUndermineTets class
//	skinCutUndermineTets _incisions;  // now derived from deepCut class

	struct undermineTriangle {
		unsigned int incisionConnect : 1;
		unsigned int triangle : 31;
	};
	std::vector<undermineTriangle> _undermineTriangles;  // user entered undermine texture points
	struct perioTri {
		unsigned int incisionConnect : 1;
		unsigned int periostealTriangle : 31;
	};
	std::list<perioTri> _periostealUndermineTriangles;
	fence _fence;
	json::Array _historyArray;
	json::Array::ValueVector::iterator _historyIt;	// current history command
	std::string _sceneDir, _historyDir;
	void historyAttachFailure(std::string& errorDescription);  // report failure and truncate history at just before this action.

	// next are temporary move variables set by ascii keys
	float _x,_y,_z,_u,_f,_r;
};

#endif // __SURGICALACTIONS_H__
