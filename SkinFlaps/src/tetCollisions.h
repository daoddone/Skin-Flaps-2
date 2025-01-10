#ifndef __TET_COLLISIONS__
#define __TET_COLLISIONS__

#include <vector>
#include <array>
#include "Vec2f.h"
#include "Vec3f.h"
#include "Mat3x3f.h"

// forward declarations
class materialTriangles;
class vnBccTetrahedra;
class pdTetPhysics;

class tetCollisions
{
public:
	void initSoftCollisions(materialTriangles *mt, vnBccTetrahedra *vnt);  // call after every topo change
	void findSoftCollisionPairs();  // call every physics iteration
	void addFixedCollisionSet(const std::string& levelSetFile, std::vector<int>& vertexIndices);  // call once at load
	void updateFixedCollisions(materialTriangles *mt, vnBccTetrahedra *vnt);  // must be done after every topo change
	bool empty() { return _fixedCollisionSets.empty() && _bedRays.empty(); }
	inline void setPdTetPhysics(pdTetPhysics *ptp) { _ptp = ptp; }
	tetCollisions() : _itCount(0), _initialized(false), _minTime((double)FLT_MAX), _maxTime(0.0){
		_fixedCollisionSets.clear(); _flapBotTris.clear(); 
	}
	~tetCollisions() {}

private:
	int _itCount;
	static materialTriangles *_mt;
	static vnBccTetrahedra *_vnt;
	static pdTetPhysics *_ptp;
	bool _initialized;
	Mat3x3f _rest[6];  // material inverses used to compute deformation gradients
	struct vertexRay {
		int vertex;
		Vec3f P;
		Vec3f N;
		Vec3f materialNormal;
		int restIdx;  // only 6 of these in bcc tets
	};
	std::vector<vertexRay> _bedRays;
	std::vector<int> _flapBotTris;

	struct fixedCollisionSet {
		std::string levelSetFilename;
		std::vector<int> vertices;
	};
	std::list< fixedCollisionSet> _fixedCollisionSets;

	float rayDepth(const Vec3f& Vtx, const Vec3f& nrm);

	double _minTime, _maxTime;

	float inverse_rsqrt(float number);
};
#endif  // __TET_COLLISIONS__

