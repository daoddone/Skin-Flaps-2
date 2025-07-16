//////////////////////////////////////////////////////////
// File: materialTriangles.cpp
// Author: Court Cutting, MD
// Date: 2/26/2015
// Purpose: Triangle storage class using only uniquely linked xyz position
//    and uv texture data for vertices.  Normals are not used
//    and texture seams are not allowed thereby making all
//    vertices unique. Triangle listings include not only
//    the 3 vertex indices, but also an integer material identifier.
//    An auxilliary graphics class
//    with its included vertex shaders provides vertex normal
//    doubling and tripling for graphics and user purposes.
//    Other auxilliary classes provide
//    a tissue specific fragment shader to procedurally texture
//    the model for graphics purposes.
//////////////////////////////////////////////////////////

#include <assert.h>
#include <fstream>
#include <algorithm>
#include <exception>
#include <string.h>
#include <array>
#include <sstream>
#include <list>
#include "materialTriangles.h"
#include "math3d.h"
#include "boundingBox.h"
#include "Mat2x2f.h"
#include "Mat3x3f.h"
#include "Vec3f.h"
#include <exception>

int materialTriangles::readObjFile(const char *fileName)
{ // returned error codes: 0=no error, 1=can't open file, 2=non-triangle primitive,
	// 3=bad 3D vertex line, 4=bad 3D texture line, 5=bad uvw face line, 6=exceeds 0x3fffffff vertex limit
	// Uses "s [smoothingGroup]" separators in front of face groups to separate materials. [smoothingGroup] must start at 1 as 0 means off.
    std::ifstream fin(fileName);
    if(!fin.is_open())
        return 1;
	_xyz.clear();
	_uv.clear();
	_triPos.clear();
	_triTex.clear();
	_triMat.clear();
	std::vector<float> tuv;
	std::string unparsedLine;
	std::vector<std::string> parsedLine;
	int vertexNumber=0;
	int matNow;
	int i,j,k,l;
	std::string str;
	char s[100];
	while(parseNextInputFileLine(&fin,unparsedLine,parsedLine))
	{
		if(parsedLine.empty())
			continue;
		// ignore all other .obj lines except the following for now
		if(parsedLine[0]=="v")
		{
			if(parsedLine.size()!=4)
				return 3;
			_xyz.push_back(Vec3f());
			Vec3f &v = _xyz.back();
			for(i=1; i<4; ++i)
				v[i-1] = (float)atof(parsedLine[i].c_str());
			++vertexNumber;
		}
		else if (parsedLine[0] == "usemtl")
		{
			if (parsedLine.size() != 2)
				return 3;
			matNow = atoi(parsedLine[1].c_str());
		}
		else if (parsedLine[0] == "vt")
		{
			if(parsedLine.size()!=3)
				return 4;
			_uv.push_back(Vec2f());
			_uv.back().X = (float)atof(parsedLine[1].c_str());
			_uv.back().Y = (float)atof(parsedLine[2].c_str());
		}
		else if(parsedLine[0]=="f")
		{	// always in vertexPosition/vertexTexture format. If vP/vT may skip normal. If vP//vN, texture is skipped.
			// triangles always input last after vertex positions and textures
			if (_uv.empty()) {  // create unique texture for each vertex
				std::cout << "Error reading .obj file: " << fileName << " . No texture coordinates specified.\n";
				return 4;
			}
			int numVerts = (int)parsedLine.size();
			if(numVerts > 5)  // we'll now allow quads and tesselate here
				return 2;
			int vIn[4][2];
			for(i=1; i<numVerts; ++i)
			{
				strncpy(s,parsedLine[i].c_str(),99);
				k=0; l=0;
				for(j=0; j<2; ++j)  // ignore normals
				{
					while(s[l]!='/' && s[l]!='\0')
						++l;
					str.clear();
					while(k!=l)
					{
						str.push_back(s[k]);
						++k;
					}
					vIn[i-1][j] = atoi(str.c_str()) - 1;	// remember indexes in obj files start at 1
					++k; ++l;
				}
			}
			std::array<int, 3> p3, t3;
			for (int k = 0; k < 3; ++k) {
				p3[k] = vIn[k][0];
				t3[k] = vIn[k][1];
			}
			_triPos.push_back(p3);
			_triTex.push_back(t3);
			_triMat.push_back(matNow);
			if (numVerts > 4) {
				for (int k = 0; k < 3; ++k) {
					p3[k] = vIn[(k+2) % 4][0];
					t3[k] = vIn[(k+2) % 4][1];
				}
				_triPos.push_back(p3);
				_triTex.push_back(t3);
				_triMat.push_back(matNow);

			}
		}
		else
			continue;
	}
	fin.close();
	if(	vertexNumber>0x3fffffff) {
		return 6;
	}
	// next line minimizes number of program switches on graphics card.
	// only done on startup as later triangle indices must remain unique for incision processing
	// trim excess capacity?  Maybe not.  Only going to grow requiring realloc
	return 0;
}

bool materialTriangles::parseNextInputFileLine(std::ifstream *infile, std::string &unparsedLine, std::vector<std::string> &parsedLine)
{
	if(infile->eof())
	{
		infile->close();
		return false;
	}
	if(!infile->is_open())
		return false;
	char s[400];
	infile->getline(s,399);
	unparsedLine.assign(s);
	if(unparsedLine=="")
	{
		parsedLine.clear();
		return true;
	}
	parsedLine.clear();
	std::string substr;
	int start=0,next=0;
	while(next<399 && s[next]!= '\0')
	{
		// advance to whitespace
		while(next<399 && s[next]!='\0' && s[next]!=' ' && s[next]!='\t')
			++next;
		substr.assign(s+start,s+next);
		parsedLine.push_back(substr);
		while(s[next]==' ' || s[next]=='\t')
			++next;
		start = next;
	}
	return true;
}

bool materialTriangles::writeObjFile(const char *fileName, const char* materialFileName)
{
	std::string title(fileName);
	if(title.rfind(".obj")>title.size())
		title.append(".obj");
	std::ofstream fout(title.c_str());
    if(!fout.is_open())
        return false;
	char s[400];
	std::string line;
	if (materialFileName != nullptr) {
		line = "mtllib ";
		line.append(materialFileName);
		if (line.rfind(".mtl") > title.size())
			line.append(".mtl");
		line.append("\n");
		fout << line;
	}
	std::vector<int> vIdx, tIdx;
	vIdx.assign(_xyz.size(), -1);
	tIdx.assign(_uv.size(), -1);
	// only save triangles and vertices that haven't been deleted
	for (size_t i = 0; i < _triPos.size(); ++i) {
		if (_triMat[i] < 0)  // skip deleted triangles
			continue;
		for (int j = 0; j < 3; ++j) {
			vIdx[_triPos[i][j]] = 1;
			tIdx[_triTex[i][j]] = 1;
		}
	}
	int vn = 1, tn = 1;  // .obj indices start at 1 , not 0
	for (size_t n = vIdx.size(), i = 0; i < n; ++i) {
		if (vIdx[i] > 0)
			vIdx[i] = vn++;
	}
	for (size_t n = tIdx.size(), i = 0; i < n; ++i) {
		if (tIdx[i] > 0)
			tIdx[i] = tn++;
	}
	for (size_t n=vIdx.size(), i = 0; i < n; ++i) {
		if (vIdx[i] < 0)
			continue;
		sprintf(s, "v %f %f %f\n", _xyz[i][0], _xyz[i][1], _xyz[i][2]);
		line.assign(s);
		fout.write(line.c_str(), line.size());
	}
	for (size_t n=tIdx.size(), i = 0; i < n; ++i) {
		if (tIdx[i] < 0)
			continue;
		sprintf(s, "vt %f %f\n", _uv[i][0], _uv[i][1]);
		line.assign(s);
		fout.write(line.c_str(), line.size());
	}
	std::map<int, int> matSmooth;
	int smoothNum=1, lastMaterial = -1;
	for(size_t n=_triMat.size(), i=0; i<n; ++i)	{
		if (_triMat[i] != lastMaterial) {  // new material/shading group
			auto pr = matSmooth.insert(std::make_pair(_triMat[i], smoothNum));
			if (pr.second) {
				++smoothNum;
				++pr.first->second;
			}
			lastMaterial = _triMat[i];
			sprintf(s, "usemtl %d\n", lastMaterial);
			line.assign(s);
			fout.write(line.c_str(), line.size());
			sprintf(s, "s %d\n", pr.first->second);
			line.assign(s);
			fout.write(line.c_str(), line.size());
		}
		sprintf(s, "f %d/%d %d/%d %d/%d\n", vIdx[_triPos[i][0]], tIdx[_triTex[i][0]], vIdx[_triPos[i][1]], tIdx[_triTex[i][1]], vIdx[_triPos[i][2]], tIdx[_triTex[i][2]]);
		line.assign(s);
		fout.write(line.c_str(),line.size());	}
	fout.close();
	return true;
}

bool materialTriangles::getBarycentricProjection(const int triangle, const float(&xyz)[3], float(&uv)[2])
{	// for position xyz return barycentric uv projection into triangle
	float *p,*q;
	int *t = _triPos[triangle].data();
	p = vertexCoordinate(t[0]);
	q = vertexCoordinate(t[1]);
	Vec3f u,v,xmp;
	u.set(q[0]-p[0],q[1]-p[1],q[2]-p[2]);
	q = vertexCoordinate(t[2]);
	v.set(q[0]-p[0],q[1]-p[1],q[2]-p[2]);
	xmp.set(xyz[0]-p[0],xyz[1]-p[1],xyz[2]-p[2]);
	float a,b,c,d;
	a=u[0]*u[0]+u[1]*u[1]+u[2]*u[2];
	b=u[0]*v[0]+u[1]*v[1]+u[2]*v[2];
	c=v[0]*v[0]+v[1]*v[1]+v[2]*v[2];
	if(fabs(d=b*b-a*c)<1e-16f)	{	// degenerate triangle
		uv[0]=0.0; uv[1]=0.0f;
		return false; }
	uv[1] = ((u*b - v*a)*xmp)/d;
	uv[0] = (xmp*u - uv[1]*b)/a;
	return true;
}

void materialTriangles::getBarycentricTexture(const int triangle, const float (&uv)[2], float (&texture)[2])
{
	int *tr = &_triTex[triangle][0];
	float p=1.0f-uv[0]-uv[1],*t0=getTexture(tr[0]),*t1=getTexture(tr[1]),*t2=getTexture(tr[2]);
	for(int i=0; i<2; ++i)
		texture[i] = t0[i]*p + uv[0]*t1[i] + uv[1]*t2[i];
}

void materialTriangles::getBarycentricPosition(const int triangle, const float (&uv)[2], float (&xyz)[3])
{	// for barycentric uv in triangle returns position in xyz
	float *p,*q;
	int *t = &_triPos[triangle][0];
	p = vertexCoordinate(t[0]);
	q = vertexCoordinate(t[1]);
	Vec3f u,v,r;
	u.set(q[0]-p[0],q[1]-p[1],q[2]-p[2]);
	q = vertexCoordinate(t[2]);
	v.set(q[0]-p[0],q[1]-p[1],q[2]-p[2]);
	r = u*uv[0] + v*uv[1];
	xyz[0]=r.X+p[0]; xyz[1]=r.Y+p[1]; xyz[2]=r.Z+p[2];
}

void materialTriangles::getBarycentricNormal(const int triangle, const float(&uv)[2], float(&nrm)[3])
{
	// look for uniform surface w triangle
//	int *tr = triangleVertices(triangle);
	Vec3f vNorm[3],bNorm;
	for (int i = 0; i < 3; ++i) 
		getMeanVertexNormal(triangle, i, vNorm[i].xyz, _triMat[triangle]);
//		triangleVertexNormal(tr[i], vNorm[i].xyz, _triMat[triangle]);
	bNorm = vNorm[1] * uv[0];
	bNorm += vNorm[2] * uv[1];
	bNorm += vNorm[0] * (1.0f - uv[0] - uv[1]);
	bNorm.normalize();
	nrm[0] = bNorm.X; nrm[1] = bNorm.Y; nrm[2] = bNorm.Z;
}

int materialTriangles::findAdjacentTriangles(bool forceCompute)
{	// computes all the adjacent triangles from raw triangle input
	// returns false if non-manifold surface is input
	if (_adjacenciesComputed && !forceCompute)
		return true;
	typedef std::set<edge, edgeTest> edgeSet;
	typedef edgeSet::iterator edgeIt;
	std::pair <edgeIt,bool> P;
	edge E;
	edgeIt ei;
	edgeSet M;
	M.clear();
	int *tnow;
	unsigned int *adjNow;
	unsigned int i,j,tcode,numtris=(unsigned int)_triPos.size();
	if (numtris < 1)
		return 1;
	_adjs.clear();
	std::array<unsigned int, 3> aa;
	aa[0] = 0x00000003; aa[1] = 0x00000003; aa[2] = 0x00000003;
	_adjs.assign(numtris, aa);
	for(i=0; i<numtris; ++i)
	{
		if(_triMat[i]<0)	// signals a deleted triangle
			continue;
		tnow = _triPos[i].data();
		adjNow = _adjs[i].data();
		for(j=0; j<3; j++) {
			if(adjNow[j]!=0x00000003)	// adjacency already computed
				continue;
			int tmp;
			if((tmp=tnow[(j+1)%3])<tnow[j]) {
				E.vtxMin = tmp;
				E.vtxMax = tnow[j];
				E.reversed = 1;	}
			else {
				E.vtxMin = tnow[j];
				E.vtxMax = tmp;
				E.reversed = 0;	}
			E.matched = 0;
			E.adjCode = (i<<2) + j; //(4i+j)
			P = M.insert(E);
			// if P.second is true, no match so edge inserted
			if(P.second==false)	// edge match found
			{
				if (P.first->reversed == E.reversed && E.vtxMin != E.vtxMax) {
					throw(std::logic_error("Triangle ordering error"));
//					return 2;  // triangle ordering error
				}
				tcode = P.first->adjCode;
				adjNow[j] = tcode;
				_adjs[tcode>>2][tcode&3] = E.adjCode;
				M.erase(P.first);
			}
			else
				adjNow[j] = 0x00000003;
		}
	}
	makeVertexToTriangleMap();
	_adjacenciesComputed = true;
	if (M.size() > 0)
		return 1;
	else
		return 0;
}

void materialTriangles::makeVertexToTriangleMap()
{
	int i, j, numtris = (int)_triPos.size();
	_vertexFace.clear();
	if (_xyz.size() < 1){  // allows processing of only topology
		int maxV = -1;
		for (i = 0; i < numtris; ++i){
			for (j = 0; j < 3; ++j){
				if (_triPos[i][j]>maxV)
					maxV = _triPos[i][j];
			}
		}
		assert(maxV > -1);
		_vertexFace.assign(maxV+1, 0x80000000);	// initially deleted
	}
	else
		_vertexFace.assign(_xyz.size(), 0x80000000);	// initially deleted
	unsigned int vnow;
	int *tnow;
	// provide each vertex with a face it is a member of
	for(i=0; i<numtris; ++i)
	{
		tnow = &(_triPos[i][0]);
		if (_triMat[i] < 0)	// signals a deleted triangle
			continue;
		for(j=0; j<3; j++)
		{
			vnow = tnow[j];
			if(_vertexFace[vnow]&0x40000000)
				continue;	// vertex first on free edge, don't change
			_vertexFace[vnow] = i;
			if(_adjs[i][j]==0x00000003)	// vertex first on free edge, lock it for easy neighbor find
				_vertexFace[vnow] |= 0x40000000;
		}
	}
}

void materialTriangles::getNeighbors(unsigned int vertex, std::vector<neighborNode> &neighbors)
{
	unsigned int trNum,adj,triStart;
	triStart = _vertexFace[vertex];
	neighbors.clear();
	if(triStart&0x80000000)	// unconnected vertex
		return;
	trNum = triStart&0x3fffffff;
	unsigned int *adjs;
	int *tnow = &(_triPos[trNum][0]);
	assert(_triMat[trNum] > -1);	// deleted triangle
	int j;
	for(j=0; j<3; ++j)
		if(tnow[j]==vertex)
			break;
	if(j>2)
		throw(std::logic_error("Program error found by materialTriangles::getNeighbors() routine."));
	adjs = &(_adjs[trNum][0]);
	// set triStart to the end adjacency code for counterclockwise traversal
	neighborNode n;
	if(triStart&0x40000000)	// started on a free edge, will end on one
	{
		triStart = 0x00000003;
		n.triangle = -1;	// code for open ring neighbors
		n.vertex = tnow[(j+1)%3];
		neighbors.push_back(n);
	}
	else	// create adjacency code of starting edge
		triStart = (trNum<<2)+j;
	n.triangle = trNum;
	n.vertex = tnow[(j+2)%3];
	neighbors.push_back(n);
	adj = adjs[(j+2)%3];
	while(adj!=triStart)
	{
		n.triangle = adj>>2;
		tnow = &(_triPos[n.triangle][0]);
		adjs = &(_adjs[n.triangle][0]);
		j = adj&0x00000003;
		n.vertex = tnow[(j+2)%3];
		neighbors.push_back(n);
		adj = adjs[(j+2)%3];
	}
}

void materialTriangles::triangleVertexNeighbors(const int triangle, const int vertexNumber, std::vector<int>& neighborTriangles, std::vector<int>& neighborVertices) {
	assert(vertexNumber < 3);
	neighborTriangles.clear();
	neighborVertices.clear();
	unsigned int adj = _adjs[triangle][(vertexNumber + 2) % 3], aEnd;
	aEnd = adj;
	int* tr;
	do {
		neighborTriangles.push_back(adj >> 2);
		tr = _triPos[adj >> 2].data();
		neighborVertices.push_back(tr[((adj & 3) + 2) % 3]);
		adj = _adjs[adj>>2][((adj & 3) + 2) % 3];
	} while (adj != aEnd && adj != 3);
	if (adj != 3)  // usual closed manifold case
		return;
	std::list<int> frontT, frontV;
	adj = (triangle << 2) + ((vertexNumber + 2) % 3);
	int openV;
	do {
		frontT.push_front(adj >> 2);
		tr = _triPos[adj >> 2].data();
		frontV.push_front(tr[adj & 3]);
		openV = tr[((adj & 3) + 2) % 3];
		adj = _adjs[adj >> 2][((adj & 3) + 1) % 3];
	} while (adj != 3);
	frontT.push_front(-1);
	frontV.push_front(openV);
	for (int n = (int)neighborTriangles.size(), i = 0; i < n; ++i) {
		frontT.push_back(neighborTriangles[i]);
		frontV.push_back(neighborVertices[i]);
	}
	neighborTriangles.assign(frontT.begin(), frontT.end());
	neighborVertices.assign(frontV.begin(), frontV.end());
}

materialTriangles::materialTriangles(const materialTriangles& x)
{
	_triPos.assign(x._triPos.begin(),x._triPos.end());
	_triTex.assign(x._triTex.begin(), x._triTex.end());
	_triMat.assign(x._triMat.begin(), x._triMat.end());
	_xyz.assign(x._xyz.begin(), x._xyz.end());
	_uv.assign(x._uv.begin(), x._uv.end());
	_adjacenciesComputed = x._adjacenciesComputed;
	_adjs.assign(x._adjs.begin(), x._adjs.end());
	_vertexFace.assign(x._vertexFace.begin(), x._vertexFace.end());
	_name = x._name;
}

materialTriangles::materialTriangles(void)  // :_adjacenciesComputed(false)
{
}


materialTriangles::~materialTriangles(void)
{
}

int materialTriangles::linePick(const Vec3f& lineStart, const Vec3f& lineDirection, std::vector<Vec3f>& positions, std::vector<int>& triangles, std::vector<float>& params, const int onlyMaterial)
{
	std::map<float, lineHit> hits;
	rayHits(lineStart.xyz, lineDirection.xyz, hits);
	positions.clear();	triangles.clear();	params.clear();
	positions.reserve(hits.size() * 3);
	triangles.reserve(hits.size());
	params.reserve(hits.size());
	for (auto it = hits.begin(); it != hits.end(); ++it) {
		if (onlyMaterial>-1 && _triMat[it->second.triangle] != onlyMaterial)
			continue;
		params.push_back(it->first);
		triangles.push_back(it->second.triangle);
		positions.push_back(it->second.v);
	}
	return (int)params.size();
}

bool materialTriangles::localPick(const float *lineStart, const float *lineDirection, float(&position)[3], int &triangle, float (&triangleParam)[2], const int onlyMaterial)
{ // lineStart and lineDirection are both 3 element vectors
	std::map<float, lineHit> hits;
	rayHits(lineStart, lineDirection, hits);
	for (auto it = hits.begin(); it != hits.end(); ++it) {
		if (it->first < -1e-8f)
			continue;
		if (onlyMaterial > -1 && _triMat[it->second.triangle] != onlyMaterial)
			continue;;
		triangle = it->second.triangle;
		triangleParam[0] = it->second.uv.xy[0];
		triangleParam[1] = it->second.uv.xy[1];
		position[0] = it->second.v.xyz[0];
		position[1] = it->second.v.xyz[1];
		position[2] = it->second.v.xyz[2];
		return true;
	}
	return false;
}

int materialTriangles::rayHits(const float *rayStart, const float *rayDirection, std::map<float, lineHit> &hits)
{ // lineStart and lineDirection are both 3 element vectors
	findAdjacentTriangles();  // may have already been done so don't force it
	Vec3f lS(rayStart[0], rayStart[1], rayStart[2]),lD(rayDirection[0], rayDirection[1], rayDirection[2]);
	hits.clear();
	lineHit pT;
	std::map<float,lineHit>::iterator hit,hit2;
	int j,bigAxis=0;
	if(fabs(rayDirection[1])>fabs(rayDirection[0]))
		bigAxis =1;
	if(fabs(rayDirection[2])>fabs(rayDirection[bigAxis]))
		bigAxis =2;
	int *tr,i;
	float *v[3];
	float t,minimax[6],tMin,tMax,rMin,rMax;
	for(i=0; i<(int)_triPos.size(); ++i)	{
		tr = _triPos[i].data();
		if (_triMat[i]<0)
			continue;
		v[0]=vertexCoordinate(tr[0]);
		v[1]=vertexCoordinate(tr[1]);
		v[2]=vertexCoordinate(tr[2]);
		minimax[0]=minimax[1]=v[0][0];
		minimax[2]=minimax[3]=v[0][1];
		minimax[4]=minimax[5]=v[0][2];
		for(j=1; j<3; ++j) {
			if(minimax[0]>v[j][0])
				minimax[0]=v[j][0];
			if(minimax[1]<v[j][0])
				minimax[1]=v[j][0];
			if(minimax[2]>v[j][1])
				minimax[2]=v[j][1];
			if(minimax[3]<v[j][1])
				minimax[3]=v[j][1];
			if(minimax[4]>v[j][2])
				minimax[4]=v[j][2];
			if(minimax[5]<v[j][2])
				minimax[5]=v[j][2];
		}
		tMin = (minimax[bigAxis<<1]-rayStart[bigAxis])/rayDirection[bigAxis];
		tMax = (minimax[(bigAxis<<1)+1]-rayStart[bigAxis])/rayDirection[bigAxis];
		float pad = (tMax-tMin)*0.1f; // fix roundoff error problem
		for(j=0; j<3; ++j) {
			if(j==bigAxis)
				continue;
			rMin=rayStart[j]+rayDirection[j]*(tMin - pad);
			rMax=rayStart[j]+rayDirection[j]*(tMax + pad);
			if(rMin<minimax[j<<1] && rMax<minimax[j<<1])
				break;
			if(rMin>minimax[(j<<1)+1] && rMax>minimax[(j<<1)+1])
				break;
		}
		if(j<3)
			continue;
		if(rayTriangleIntersection(lS, lD, i, t, pT.uv.xy, pT.v)) {
			pT.triangle = i;
			hits.insert(std::make_pair(t,pT));
		}
	}
	std::set<int> neiSet;
	auto addVertexNeighbors = [&](int tri, int idx) {
		std::vector<int> v, t;
		triangleVertexNeighbors(tri, idx, t, v);
		for (auto& tr : t) {
			if (tr < 0)
				continue;
			neiSet.insert(tr);
		}
	};
	// remove duplicated triangle edge and vertex repeats
	for(hit=hits.begin(); hit!=hits.end(); ++hit)	{
		hit2 = hit;
		++hit2;
		while (hit2 != hits.end()) {
			if (hit2->first - hit->first < 1e-4f) {
				unsigned int atr;
				neiSet.clear();
				if (hit2->second.uv[0] < 1e-5f){
					if (hit2->second.uv[1] < 1e-5f)
						addVertexNeighbors(hit2->second.triangle, 0);
					else if (hit2->second.uv[1] > 0.9999f)
						addVertexNeighbors(hit2->second.triangle, 2);
					else {
						if ((atr = _adjs[hit2->second.triangle][2]) != 3)
							neiSet.insert(atr >> 2);
					}
				}
				else if (hit2->second.uv[1] < 1e-5f){
					if (hit2->second.uv[0] > 0.9999f)
						addVertexNeighbors(hit2->second.triangle, 1);
					else {
						if ((atr = _adjs[hit2->second.triangle][0]) != 3)
							neiSet.insert(atr >> 2);
					}
				}
				else if (hit2->second.uv[0] + hit2->second.uv[1] > 0.9999f){
					if((atr = _adjs[hit2->second.triangle][1]) != 3)
						neiSet.insert(atr >> 2);
				}
				else
					;
				if (!neiSet.empty() && neiSet.find(hit->second.triangle) != neiSet.end())
					hit2 = hits.erase(hit2);
				else
					++hit2;
			}
			else
				break;
		}
	}
	return (int)hits.size();
}

bool materialTriangles::rayTriangleIntersection(const Vec3f &rayOrigin, const Vec3f &rayDirection, const int triangle, float &rayParam, float(&triParam)[2], Vec3f &intersect)
{
	Vec3f b, r, U, V, t[3];
	int *tr = triangleVertices(triangle);
	for (int i = 0; i < 3; ++i)
		getVertexCoordinate(tr[i], t[i].xyz);
	b = rayOrigin - t[0];
	U = t[1] - t[0];
	V = t[2] - t[0];
	Mat3x3f m(-rayDirection, U, V);
	r = m.Robust_Solve_Linear_System(b);
	if (r.Y<-1e-4f || r.Z<-1e-4f || r.Y >1.0001f || r.Z >1.0001f || r.Y + r.Z>1.0001f) // allows for roundoff error
		return false;
	rayParam = r.X;
	triParam[0] = r.Y;
	triParam[1] = r.Z;
	intersect = t[0] + U*triParam[0] + V*triParam[1];
	return true;
}

void materialTriangles::getTriangleNormal(int triangle, Vec3f &normal, bool normalize)
{
	int *tr = _triPos[triangle].data();
	Vec3f v0,v1;
	v0 = _xyz[tr[1]] - _xyz[tr[0]];
	v1 = _xyz[tr[2]] - _xyz[tr[0]];
	normal = v0^v1;
	if(normalize)
		normal.normalize();
}

void materialTriangles::closestPoint(const float(&xyz)[3], int& triangle, float(&uv)[2], int onlyMaterial){  // closest barycentric position to point xyz
	Vec3f P;
	float dsq, minDsq = FLT_MAX;
	P.set(xyz);
	for (int n = (int)_triPos.size(), j, i = 0; i < n; ++i) {
		if (_triMat[i] < 0 || (onlyMaterial > -1 && _triMat[i] != onlyMaterial))
			continue;
		Vec3f T[3];
		for (j = 0; j < 3; ++j)
			T[j] = _xyz[_triPos[i][j]];
		T[1] -= T[0];
		T[2] -= T[0];
		Mat2x2f M(T[1] * T[1], T[2] * T[1], 1.0f, T[2] * T[2]);
		M.x[2] = M.x[1];
		T[0] -= P;
		Vec2f R = M.Robust_Solve_Linear_System(Vec2f(-T[0] * T[1], -T[0] * T[2]));
		if (R[0] < 0.0f)
			R[0] = 0.0f;
		else if (R[0] > 1.0f)
			R[0] = 1.0f;
		else
			;
		if (R[1] < 0.0f)
			R[1] = 0.0f;
		else if (R[1] > 1.0f)
			R[1] = 1.0f;
		else
			;
		T[0] += T[1] * R[0] + T[2] * R[1];
		dsq = T[0].length2();
		if (dsq < minDsq) {
			minDsq = dsq;
			triangle = i;
			uv[0] = R[0];
			uv[1] = R[1];
		}
	}
}

int materialTriangles::splitTriangleEdge(int triangle, int edge, const float parameter)
{	// Splits a triangle along edge(0-2) by parameter(0-1).
	// Creates 1 or 2 new triangles and one new vertex. Returns new vertex number of the input triangle split.
	// Does fix adjacency array so getNeighbors() works and texture interpolated.
	// Definitely should call getTriangleAdjacencies() ASAP.
	assert(0.0f<=parameter && 1.0f>=parameter);
	int *trVerts = _triPos[triangle].data(), * trTex = _triTex[triangle].data();
	if (_triMat[triangle]<0)
		return -1;
	int tn,newVert= addVertices(1);
	int ve=trVerts[edge],ve1=trVerts[(edge+1)%3];
	float gv[3], tx[2];
	float *gvp=vertexCoordinate(ve), *txp=getTexture(trTex[edge]);
	gv[0]=(1.0f-parameter)*gvp[0]; gv[1]=(1.0f-parameter)*gvp[1]; gv[2]=(1.0f-parameter)*gvp[2];
	tx[0] = (1.0f - parameter) * txp[0]; tx[1] = (1.0f - parameter) * txp[1];
	gvp = vertexCoordinate(ve1);
	gv[0]+=parameter*gvp[0]; gv[1]+=parameter*gvp[1]; gv[2]+=parameter*gvp[2];
	txp = getTexture(trTex[(edge + 1) % 3]);
	tx[0] += parameter * txp[0]; tx[1] += parameter * txp[1];
	setVertexCoordinate(newVert,gv);
	int tx0 = addTexture();
	setTexture(tx0, tx);
	int v[3], tex[3];
	v[1] = trVerts[(edge + 1) % 3];
	tex[1] = trTex[(edge + 1) % 3];
	trVerts[(edge + 1) % 3] = newVert;
	trTex[(edge + 1) % 3] = tx0;
	v[0] = newVert;	v[2] = trVerts[(edge + 2) % 3];
	tex[0] = tx0;	tex[2] = trTex[(edge + 2) % 3];
	tn = addTriangle(v, _triMat[triangle], tex);	// invalidates old _tris and _adjs pointers
	if(_adjs[triangle][edge] == 0x00000003) {
		_adjs[tn][0] = 0x00000003;
		unsigned int adjTE = _adjs[triangle][(edge + 1) % 3];
		_adjs[tn][1] = adjTE;
		if(adjTE != 3)
			_adjs[adjTE>>2][adjTE & 3] = (tn << 2) + 1;
		_adjs[tn][2] = (triangle << 2) + ((edge + 1) % 3);
		_adjs[triangle][(edge + 1) % 3] = (tn << 2) + 2;
		if((_vertexFace[v[1]]&0x3fffffff)==triangle)
			_vertexFace[v[1]] = (tn | 0x40000000);	// first vertex on a free edge
		_vertexFace[newVert] = (tn | 0x40000000);
		return newVert;
	}
	int tx1 = tx0;
	unsigned int* trAdjs = _adjs[triangle].data();
	int *trVertsA = _triPos[trAdjs[edge] >> 2].data(), * trTexA = _triTex[trAdjs[edge] >> 2].data();
	unsigned int* trAdjsA = _adjs[trAdjs[edge] >> 2].data();
	int ea = trAdjs[edge] & 0x00000003, ta = trAdjs[edge] >> 2;
	if (trTexA[ea] != tex[1] || trTexA[(ea + 1) % 3] != trTex[edge]) {  // texture seam at edge
		txp = getTexture(trTexA[ea]);
		tx[0] = parameter * txp[0]; tx[1] = parameter * txp[1];
		txp = getTexture(trTexA[(ea + 1) % 3]);
		tx[0] += (1.0f - parameter) * txp[0]; tx[1] += (1.0f - parameter) * txp[1];
		tx1 = addTexture();
		setTexture(tx1, tx);
	}
	v[1] = trVertsA[(ea + 1) % 3];
	tex[1] = trTexA[(ea + 1) % 3];
	trVertsA[(ea + 1) % 3] = newVert;
	trTexA[(ea + 1) % 3] = tx1;
	v[0] = newVert;	v[2] = trVertsA[(ea + 2) % 3];
	tex[0] = tx1;	tex[2] = trTexA[(ea + 2) % 3];
	int tna = addTriangle(v, _triMat[ta], tex);	// invalidates old _tris and _adjs pointers
	trAdjs = _adjs[triangle].data();
	trAdjsA = _adjs[trAdjs[edge] >> 2].data();
	// new adj assignments
	unsigned int ae1,aa1;
	trAdjs[edge] = (tna<<2);
	trAdjsA[ea] = (tn << 2);
	ae1 = trAdjs[(edge+1)%3];
	aa1 = trAdjsA[(ea + 1) % 3];
	trAdjs[(edge+1)%3] = (tn<<2) + 2;
	trAdjsA[(ea + 1) % 3] = (tna << 2) + 2;
	if(ae1 != 3)
		_adjs[ae1>>2][ae1 & 3] = (tn << 2) + 1;
	if (aa1 != 3)
		_adjs[aa1 >> 2][aa1 & 3] = (tna << 2) + 1;
	// now set adjacencies for tn and tna
	trAdjs = _adjs[tn].data();
	trAdjsA = _adjs[tna].data();
	trAdjs[0] = (ta << 2) + ea;
	trAdjsA[0] = (triangle << 2) + edge;
	trAdjs[1] = ae1;
	trAdjsA[1] = aa1;
	trAdjs[2] = (triangle << 2) + ((edge + 1) % 3);
	trAdjsA[2] = (ta << 2) + ((ea + 1) % 3);
	// new vertexFace assignments
	_vertexFace[newVert] = triangle;
	if((_vertexFace[ve1]&0x3fffffff)==triangle) {
		if((_vertexFace[ve1]&0x40000000)>0)
			_vertexFace[ve1] = tn|0x40000000;
		else
			_vertexFace[ve1] = tn;
	}
	if((_vertexFace[ve]&0x3fffffff)==ta) { // va1
		if((_vertexFace[ve]&0x40000000)>0)
			_vertexFace[ve] = tna|0x40000000;
		else
			_vertexFace[ve] = tna;
	}
	return newVert;
}

int materialTriangles::addNewVertexInMidTriangle(int triangle, const float (&uvParameters)[2])
{	// creates 2 new triangles and one new vertex. Returns new vertex position number.
	// Input uvParameters[2] are parameters along vectors t1-t0 and t2-t0.
	// Does fix adjacency array so getNeighbors() works and texture & positions interpolated.
	// Definitely should getTriangleAdjacencies() ASAP.
	assert(uvParameters[0]>=0.0f && uvParameters[0]<=1.0f);
	assert(uvParameters[1]>=0.0f && uvParameters[1]<=1.0f);
	assert(uvParameters[0]+uvParameters[1]<=1.0001f);
	if (_triMat[triangle] < 0) {
		throw(std::logic_error("Trying to add a vertex into a deleted triangle."));
		return -1;
	}
	int* trVerts = triangleVertices(triangle);
	if(uvParameters[0]<0.0002f && uvParameters[1]<0.0002f)  // COURT check that these 12 lines create a deep point
		return trVerts[0];
	if(uvParameters[0]>0.9998f)
		return trVerts[1];
	if(uvParameters[1]>0.9998f)
		return trVerts[2];
	if(uvParameters[0]<0.0002f)
		return splitTriangleEdge(triangle,2,1.0f-uvParameters[1]);
	if(uvParameters[1]<0.0002f)
		return splitTriangleEdge(triangle,0,uvParameters[0]);
	if(uvParameters[0]+uvParameters[1]>0.9998f)
		return splitTriangleEdge(triangle,1,1.0f-uvParameters[0]);
	// now we know we will add a vertex and two triangles. These operations invalidate pointers due to possible reallocation.
	int* trTex = triangleTextures(triangle);
	int v[3],v0=trVerts[0],v1=trVerts[1],oldVert=trVerts[2], tx0 = trTex[0], tx1 = trTex[1], oldTx = trTex[2];
	int ret = addVertices(), rTx = addTexture();
	unsigned int *trAdjs = _adjs[triangle].data();
	unsigned int a1=trAdjs[1],a2=trAdjs[2];
	float p,*pv = vertexCoordinate(v0);
	p = 1.0f - uvParameters[0] - uvParameters[1];
	float vec[3] = {p*pv[0],p*pv[1],p*pv[2]};
	pv = getTexture(trTex[0]);  // vertexTexture(v0);
	float tx[2]={p*pv[0],p*pv[1]};
	for(int i=0; i<2; ++i)	{
		pv = getTexture(trTex[i+1]);
		tx[0] += pv[0]*uvParameters[i];
		tx[1] += pv[1]*uvParameters[i];
		pv = vertexCoordinate(trVerts[i+1]);
		vec[0]+=pv[0]*uvParameters[i]; vec[1]+=pv[1]*uvParameters[i]; vec[2]+=pv[2]*uvParameters[i];
	}
	setVertexCoordinate(ret,vec);
	setTexture(rTx,tx);
	// assign vertices
	trVerts[2] = ret;
	trTex[2] = rTx;
	int t[3];
	v[0] = ret; v[1] = v1; v[2] = oldVert;
	t[0] = rTx; t[1] = trTex[1]; t[2] = oldTx;
	int t2, t1 = addTriangle(v, _triMat[triangle], t);  // invalidates _tris and _adj pointers and iterators
	trTex = triangleTextures(triangle);
	v[0] = ret; v[2] = v0; v[1] = oldVert;
	t[0] = rTx; t[2] = trTex[0]; t[1] = oldTx;
	t2 = addTriangle(v, _triMat[triangle], t);  // invalidates _tris and _adj pointers and iterators
	// assign adjs
	_adjs[triangle][1] = t1 << 2;
	_adjs[triangle][2] = (t2 << 2) + 2;
	_adjs[t1][0] = (triangle << 2) + 1;
	_adjs[t1][1] = a1;
	_adjs[t1][2] = t2<<2;
	_adjs[t2][0] = (t1 << 2) + 2;
	_adjs[t2][1] = a2;
	_adjs[t2][2] = (triangle<<2)+2;
	if(a1!=3)
		_adjs[a1>>2][a1&3] = (t1<<2)+1;
	if(a2!=3)
		_adjs[a2>>2][a2&3] = (t2<<2)+1;
	// assign vertexFace
	if ((_vertexFace[oldVert] & 0x3fffffff) == triangle) {
		_vertexFace[oldVert]=t2;
		if(a2==0x00000003)
			_vertexFace[oldVert] |= 0x40000000;
	}
	if((_vertexFace[v1]&0x3fffffff)==triangle)	{
		_vertexFace[v1]=t1;
		if(a1==0x00000003)
			_vertexFace[v1] |= 0x40000000;
	}
	_vertexFace[ret]=triangle;
	// 	_vertexFace[v0] can stay unchanged
	return ret;
}

int materialTriangles::addTriangle(const int(&vertices)[3], const int material,  const int(&textures)[3])
{
	int retval = (int)_triPos.size();
	std::array<int, 3> pos, tex;
	for (int i = 0; i < 3; ++i) {
		pos[i] = vertices[i];
		tex[i] = textures[i];
	}
	_triPos.push_back(pos);
	_triTex.push_back(tex);
	_triMat.push_back(material);
	if (!_adjs.empty()) {
		std::array<unsigned int, 3> a;
		a.fill(3);
		_adjs.push_back(a);
	}
	_adjacenciesComputed = false;
	return retval;
}

int materialTriangles::addVertices(int numberToAdd)
{
	int retval = (int)_xyz.size();
	for(unsigned int i=0; i<(unsigned)numberToAdd; ++i)
	{
		_xyz.push_back(Vec3f());
		// in new version texture not present.  Force addTexture() instead
		if(!_vertexFace.empty())
			_vertexFace.push_back(0x80000000);
	}
	_adjacenciesComputed = false;
	return retval;
}

void materialTriangles::getMeanVertexNormal(const int triangle, const int index,  float(&normal)[3], int onlyMaterial, bool normalize)
//void materialTriangles::getMeanVertexNormal(int vertex, float(&normal)[3], int onlyMaterial)
{  // if onlyMaterial>-1 only use neighbor triangles with material == onlyMaterial
	assert(index < 3);
	if(!_adjacenciesComputed)	findAdjacentTriangles();
	std::vector<int> tris, verts;
	triangleVertexNeighbors(triangle, index, tris, verts);
	if(verts.size()<2) {
		normal[0]=0.0f; normal[1]=0.0f; normal[2]=0.0f;
		return; }
	Vec3f last,now,p,mean(0.0f,0.0f,0.0f);
	getVertexCoordinate(_triPos[triangle][index], p.xyz);
	int lastV, idx = 0;
	if(tris.front()<0) {
		lastV = verts.front();
		++idx;
	}
	else
		lastV = verts.back();
	getVertexCoordinate(lastV, last.xyz);
	last -= p;
	while(idx < verts.size()) {
		getVertexCoordinate(verts[idx], now.xyz);
		now -= p;
		if (onlyMaterial<0 || _triMat[tris[idx]] == onlyMaterial)
			mean += last^now;
		last = now;
		++idx;
	}
	if(normalize)
		mean.normalize();
	normal[0]=mean.X; normal[1]=mean.Y; normal[2]=mean.Z;
}

void materialTriangles::clear()
{
	_triPos.clear();
	_triTex.clear();
	_triMat.clear();
	_xyz.clear();
	_uv.clear();
	_adjacenciesComputed= false;
	_adjs.clear();
	_vertexFace.clear();
	_name.assign("");
}

float materialTriangles::getDiameter() {
	findAdjacentTriangles(false);
	boundingBox<float> bb;
	bb.Empty_Box();
	Vec3f v, max;
	for (int n = (int)_xyz.size(), i = 0; i < n; ++i) {
		getVertexCoordinate(i, v.xyz);
		bb.Enlarge_To_Include_Point(v.xyz);
	}
	bb.Minimum_Corner(v.xyz);
	bb.Maximum_Corner(max.xyz);
	return (max - v).length();
}
