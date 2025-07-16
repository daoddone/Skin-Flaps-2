//////////////////////////////////////////////////////////
// File: surgGraphics.cpp
// Author: Court Cutting, MD
// Date: 3/24/21  Original 3/3/2015
// Purpose: Takes as input a materialTriangles class and makes it visible
//    on an openGL canvas.  It creates hard normal edges on
//    the model given an input texture file for the top skin. Lip side
//    texturing, bottom fat and deep muscle are textured procedurally in
//    the fragment shader.
//////////////////////////////////////////////////////////

#include <algorithm>
#include <set>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <assert.h>
#include "Vec3f.h"
#include "lightsShaders.h"
#include "boundingBox.h"
#include "gl3wGraphics.h"
#include "surgGraphics.h"

GLuint incisionLines::_incisionBufferObjects[2] = { 0xffffffff, 0xffffffff };
GLuint incisionLines::_incisionVertexArrayBufferObject = 0xffffffff;


const GLchar *surgGraphics::skinVertexShader = "#version 150 core\n"
	"in vec4 vVertex;"
	"in vec3 vNormal;"
	"in vec3 vTangent;"
	"in vec2 vTexture;"
	"uniform mat4   mvpMatrix;"
	"uniform mat4   mvMatrix;"
	"uniform mat3   normalMatrix;"
	"uniform vec3   vLightPosition;"
	// next 2 are for normal mapping per Randi Rost
	"smooth out vec3 vLightDir;"
	"smooth out vec3 vEyeDir;"
	"smooth out vec2 vTexCoords;"
	"void main(void) "
	"{"
	"   vEyeDir = vec3(mvMatrix * vVertex);"
	"	vec3 n = normalize(normalMatrix * vNormal);\n"
	"	vec3 t = normalize(normalMatrix * vTangent);\n"
	"	vec3 b = cross(n,t);\n"
	"	vec3 v;\n"
	"	v.x = dot(vLightPosition,t);\n"
	"	v.y = dot(vLightPosition,b);\n"
	"	v.z = dot(vLightPosition,n);\n"
	"	vLightDir = normalize(v);\n"
	"	v.x = dot(vEyeDir,t);\n"
	"	v.y = dot(vEyeDir,b);\n"
	"	v.z = dot(vEyeDir,n);"
	"	vEyeDir = normalize(v);"
	"	vTexCoords = vTexture;"
	"   gl_Position = mvpMatrix * vVertex;"
	"}";

const GLchar *surgGraphics::skinFragmentShader =
	// Adapted from Randi Rost and Bill Licea-Kane
	// OpenGL Shading Language 3rd edition
	"#version 150 core\n"

	// next line only necessary in web_gl. Has been in openGL since 2.0
//	"#extension GL_OES_standard_derivatives : enable"

	"out vec4 vFragColor;"
	"uniform vec4 ambientColor;"
	"uniform vec4 diffuseColor;"
	"uniform sampler2D colorMap;"
	"uniform sampler2D normalMap;"
	"uniform int material;"
	"smooth in vec3 vLightDir;"
	"smooth in vec3 vEyeDir;"
	"smooth in vec2 vTexCoords;"
	"/* Description : Array and textureless GLSL 2D & 3D simplex noise functions."
	"//      Author : Ian McEwan, Ashima Arts."
	"//  Maintainer : ijm"
	"//     Lastmod : 20110822 (ijm)"
	"//     License : Copyright (C) 2011 Ashima Arts. All rights reserved."
	"//               Distributed under the MIT License. See LICENSE file."
	"//               https://github.com/ashima/webgl-noise */"
	"vec3 mod289(vec3 x) {"
	"  return x - floor(x * (1.0 / 289.0)) * 289.0;}"
	"vec2 mod289(vec2 x) {"
	"  return x - floor(x * (1.0 / 289.0)) * 289.0;}"
	"vec3 permute(vec3 x) {"
	"  return mod289(((x*34.0)+1.0)*x);}"
	"vec4 taylorInvSqrt(vec4 r){"
	"  return 1.79284291400159 - 0.85373472095314 * r;}"
	"float snoise(vec2 v)  {"
	"  const vec4 C = vec4(0.211324865405187,"  // (3.0-sqrt(3.0))/6.0
	"                      0.366025403784439,"  // 0.5*(sqrt(3.0)-1.0)
	"                     -0.577350269189626,"  // -1.0 + 2.0 * C.x
	"                      0.024390243902439);" // 1.0 / 41.0
	// First corner
	"  vec2 i  = floor(v + dot(v, C.yy) );"
	"  vec2 x0 = v -   i + dot(i, C.xx);"
	// Other corners
	"  vec2 i1;"
	  //i1.x = step( x0.y, x0.x ); // x0.x > x0.y ? 1.0 : 0.0
	  //i1.y = 1.0 - i1.x;
	"  i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);"
	  // x0 = x0 - 0.0 + 0.0 * C.xx ;
	  // x1 = x0 - i1 + 1.0 * C.xx ;
	  // x2 = x0 - 1.0 + 2.0 * C.xx ;
	"  vec4 x12 = x0.xyxy + C.xxzz;"
	"  x12.xy -= i1;"
	// Permutations
	"  i = mod289(i);" // Avoid truncation effects in permutation
	"  vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))"
	"		+ i.x + vec3(0.0, i1.x, 1.0 ));"
	"  vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);"
	"  m = m*m ;"
	"  m = m*m ;"
	// Gradients: 41 points uniformly over a line, mapped onto a diamond.
	// The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)
	"  vec3 x = 2.0 * fract(p * C.www) - 1.0;"
	"  vec3 h = abs(x) - 0.5;"
	"  vec3 ox = floor(x + 0.5);"
	"  vec3 a0 = x - ox;"
	// Normalise gradients implicitly by scaling m
	// Approximation of: m *= inversesqrt( a0*a0 + h*h );
	"  m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );"
	// Compute final noise value at P
	"  vec3 g;"
	"  g.x  = a0.x  * x0.x  + h.x  * x0.y;"
	"  g.yz = a0.yz * x12.xz + h.yz * x12.yw;"
	"  return 130.0 * dot(m, g);}"

	// next routine gets fat from 4 surrounding noise values [-1,1]
	"void getFat(in vec4 nei, out vec4 fatColor, out vec3 normDelta, out float specMult) {"
	"  float h;"
	"  for(int i=0; i<4; ++i){"
	"    h = 1.0 - abs(nei[i]);"
	"    h *= h;"
	"    nei[i] = 1.0 - h; }"
	"  h = 0;"
	"  for(int i=0; i<4; ++i)"
	"    h += nei[i];"
	"  h *= 0.25;"
	"  vec2 p;"
	"  p.x = nei[1]-nei[0];"
	"  p.y = nei[2]-nei[3];"
	"  p *= 130.0;"
	"  p = clamp(p,-1.0,1.0);"
	"  float d,f;"
	"  d = dot(p,p);"
	"  f = inversesqrt(d+1.0);"
	"  p.x = -p.x;"
	"  normDelta = vec3(p,1.0)*f;"
	"  float fatRed, fatGreen, fatBlue;"
	"  if(h<0.04) {"
	"    specMult = 0.2;"
	"    fatRed = (1.0-h)*0.4;"
	"    fatBlue = 0.0;"
	"    fatGreen = (1.0-h)*0.2; }"
	"  else {"
	"    specMult = 1.0;"
	"    fatRed = 0.5 + h*0.8;"
	"    fatBlue = 0.15 + h*0.3;"
	"    fatGreen = 0.35 + h*0.8; }"
	"  fatColor = vec4(fatRed, fatGreen, fatBlue, 1.0); }"

	"void main(void)"
	"{"
	// always lighting with white light
	"	const float ambientVal = 0.1;"
	"	const float	fatIncr = 0.5/1024.0;"
	"   vec2 fatD = vec2(fatIncr,0.0);"
	"   vec2 faceUV;"
	"	float lightVal = ambientVal;"
	"	float sn,h,dm=6.0,specMult = 0.5;"
	"	const float diffuseVal = 0.9;"
	"	vec3 normDelta = vec3(0.0, 0.0, 1.0);"
	"	vec3 litColor = vec3(1.0, 1.0, 1.0);"
	"	vec4 nei;"
	"	if(material>0) {"
	"		if(material==4) {"  // periosteum
	"				vFragColor = vec4(0.984, 0.9255, 0.855, 1.0);"
	"				specMult = 0.6; }"
	"		else if(material==2) {"  // flap bottom fat
	"			faceUV = vTexCoords*50.0f;"
	"			nei[0] = snoise(faceUV - fatD);"
	"			nei[1] = snoise(faceUV + fatD);"
	"			nei[2] = snoise(faceUV - fatD.yx);"
	"			nei[3] = snoise(faceUV + fatD.yx);"
	"			getFat(nei,vFragColor,normDelta,specMult);"
	"		} "
	"		else {"  // middle. material==1
	"			faceUV = vTexCoords;"
	"			sn = snoise(vec2(dm*faceUV.t*3.0,0.5));"
	"			float val = (sn+1.0)*0.5;"
	"			if(0.415 + 0.2*val < faceUV.s) {"  // fat
	"				faceUV *= vec2(6.4,dm*4.7);"
	"				nei[0] = snoise(faceUV + fatD);"
	"				nei[1] = snoise(faceUV - fatD);"
	"				nei[3] = snoise(faceUV + fatD.yx);"
	"				nei[2] = snoise(faceUV - fatD.yx);"
	"				getFat(nei,vFragColor,normDelta,specMult);"
	"			}"
	"			else if(0.40 + 0.2*val < faceUV.s) {"  // dermal-fat junction
	"				vFragColor = vec4(0.51, 0.44, 0.1412, 1.0);"
	"				specMult = 0.2; }"
	"			else {"  // dermis
	"				specMult = 0.35;"
	"				if(faceUV.s<0.05)"
	"					vFragColor = vec4(0.71, 0.57255, 0.2784, 1.0);"
	"				else if(faceUV.s<0.07)"
	"					vFragColor = vec4(0.843, 0.737, 0.51, 1.0);"
	"				else {"
	"					sn = snoise(vec2((dm*faceUV.t+0.4)*4.2,0.5));"
	"					val = (sn-0.5)*2.0;"
	"					if(0.30 + 0.05*val < faceUV.s) {"
	"						vFragColor = vec4(0.9569, 0.8902, 0.71, 1.0); }"
	"					else {"
	"						vFragColor = vec4(0.7255, 0.5059, 0.2039, 1.0);"
	"						vFragColor = vFragColor*(0.5+2.8*faceUV.s); }"
	"				}"
	"			}"
	"		}"
	"   }"
	"	else if(material==5) {" // blank end
	"	  vec4 tx1 = vec4(1.0);"
	"	  tx1.rgb = vec3(0.26,0.18,0.1);"
	"	  normDelta = vec3(1.0);"
	"	  specMult = 0.0;"
	"	  vFragColor = tx1; }"
	"	else {"	// top of skin. material==0
	"	  vec4 tx1 = texture(normalMap,vTexCoords.st);"
	"	  tx1.rgb -= vec3(0.5);"
	"	  normDelta = tx1.rgb*2.0;"
	"	  specMult = 0.11;"
	"	  vFragColor = texture(colorMap, vTexCoords.st); }"
	"	lightVal += diffuseVal*max(dot(normDelta,vLightDir), 0.0);"
	"	if(material==5) "
	"		vFragColor *= 0.8 + 0.2*lightVal;"
	"	else "
	"		vFragColor *= lightVal;"
	"	vec3 reflectDir = reflect(vLightDir,normDelta);"
	"	float spec = max(dot(vEyeDir,reflectDir),0.0);"
	"	spec = pow(spec,40.0);"
	"	spec *= specMult;"
	"	litColor = min(vFragColor.rgb + spec, vec3(1.0));"
	"	vFragColor = vec4(litColor, 1.0);"
	"}";

bool surgGraphics::setTextureFilesCreateProgram(std::vector<int> &textureIds, const char *vertexShaderFile, const char *fragmentShaderFile)
{  // must be set first
	auto readShader = [](const char *fileName, std::string &shader) ->bool{
		try{
			std::ifstream in(fileName, std::ios::in | std::ios::binary);
			in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
			if (in)	{
				std::ostringstream contents;
				contents << in.rdbuf();
				in.close();
				shader = contents.str();
			}
		}
		catch (std::ifstream::failure e) {
			std::cerr << "Could not read " << fileName << ".\n";
			return false;
		}
		return true;
	};
	std::string vShd, fShd;
	if(!readShader(vertexShaderFile, vShd))
		return false;
	if(!readShader(fragmentShaderFile, fShd))
		return false;
	std::vector<std::string> att;
	att.assign(4,std::string());
	att[0] = "vVertex";
	att[1] = "vNormal";
	att[2] = "vTangent";
	att[3] = "vTexture";
	for (int i = 0; i < (int)textureIds.size(); ++i) {
		if (!_gl3w->getTextures()->textureExists(textureIds[i]))
			return false;
	}
	if (_sn.use_count() < 1) {
		_sn = std::make_shared<sceneNode>();
		_sn->vertexArrayBufferObject = 0xffffffff;
	}
	_sn->setType(sceneNode::nodeType::MATERIAL_TRIANGLES);
	_sn->visible = true;
	_sn->setSurgGraphics(this);
	bool ret = _gl3w->addCustomSceneNode(_sn, textureIds, vShd.c_str(), fShd.c_str(), att);
	if(!ret)
		return ret;
//	_gl3w->getLightsShaders()->useGlslProgram(_sn->glslProgram);  // must be current program. This routine sets other uniforms.
	if (_sn->bufferObjects.size() != 5) {
		_sn->bufferObjects.assign(5, 0);
		glGenBuffers(5, &_sn->bufferObjects[0]);
	}
	// Vertex data - allocate reasonable initial size (will be resized in setNewTopology)
	glBindBuffer(GL_ARRAY_BUFFER, _sn->bufferObjects[0]);	// VERTEX_DATA
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 4 * 65536, NULL, GL_DYNAMIC_DRAW);  // 65K vertices * 4 floats
	// Normal data
	glBindBuffer(GL_ARRAY_BUFFER, _sn->bufferObjects[1]);	// NORMAL_DATA
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * 65536, NULL, GL_DYNAMIC_DRAW);  // 65K normals * 3 floats
	glBindBuffer(GL_ARRAY_BUFFER, _sn->bufferObjects[2]);	// TANGENT_DATA
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * 65536, NULL, GL_DYNAMIC_DRAW);  // 65K tangents * 3 floats
	// Texture coordinates
	glBindBuffer(GL_ARRAY_BUFFER, _sn->bufferObjects[3]);	// TEXTURE_DATA
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 2 * 65536, NULL, GL_DYNAMIC_DRAW);  // 65K texcoords * 2 floats
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _sn->bufferObjects[4]);	// TRIANGLE INDEX_DATA
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * 3 * 65536, NULL, GL_STATIC_DRAW);  // 65K triangles * 3 indices
	if (_sn->vertexArrayBufferObject == 0xffffffff)
		glGenVertexArrays(1,&_sn->vertexArrayBufferObject);
	// now make vertex array
	glBindVertexArray(_sn->vertexArrayBufferObject);
    // Position data
    glBindBuffer(GL_ARRAY_BUFFER, _sn->bufferObjects[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    // Normal data
    glBindBuffer(GL_ARRAY_BUFFER, _sn->bufferObjects[1]);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	// Tangent data
	glBindBuffer(GL_ARRAY_BUFFER, _sn->bufferObjects[2]);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	// Texture coordinates
	glBindBuffer(GL_ARRAY_BUFFER, _sn->bufferObjects[3]);	// TEXTURE_DATA
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, 0);
    // Indexes
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _sn->bufferObjects[4]);	// TRIANGLE INDEX_DATA */
	// never unbind a GL_ARRAY_BUFFER or GL_ELEMENT_ARRAY_BUFFER inside a vertexArrayBuffer
	glBindVertexArray(0);
	return true;
}

void surgGraphics::setNewTopology()
{
	// can't _mt.partitionTriangleMaterials() as it invalidates adjacency arrays
	_mt.findAdjacentTriangles(true);
	_tris.clear();
	_xyz1.clear();
	_uv.clear();
	_incisionLines.clear();
	auto mtta = _mt.getTextureArray();
	int n = (int)mtta.size();
	_uv.reserve(n << 1);
	for (auto uvit = mtta.begin(); uvit != mtta.end(); ++uvit) {
		_uv.push_back(uvit->X);
		_uv.push_back(uvit->Y);
	}
	_xyz1.clear();
	_xyz1.assign(n << 2, 1.0f);
	_uvPos.clear();
	_uvPos.assign(n, -1);
//	auto trArr = _mt.getPositionArray();
	_tris.reserve(n*3);
	auto trPos = _mt.getTrianglePositionArray();
	auto trMat = _mt.getTriangleMaterialArray();
	auto trTex = _mt.getTriangleTextureArray();
	for(int n = (int)trTex.size(), i=0; i<n; ++i) {
		// include possible deleted triangles so numbering matches up.
		bool valid = true;
		if (trMat[i] < 0) {
			_tris.push_back(0xffffffff);
			valid = false;
		}
		else
			_tris.push_back(trTex[i][0]);
		_tris.push_back(trTex[i][1]);
		_tris.push_back(trTex[i][2]);
		if (valid) {
			for (int j = 0; j < 3; ++j)
				_uvPos[trTex[i][j]] = trPos[i][j];
		}
	}
	getTextureSeams();
	// Vertex data
	glBindBuffer(GL_ARRAY_BUFFER, _sn->bufferObjects[0]);	// VERTEX_DATA
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*_xyz1.size(), &(_xyz1[0]), GL_DYNAMIC_DRAW);
	// Normal data
	std::vector<GLfloat> tnVec;
	tnVec.assign((_uv.size() >> 1) * 3, 0.0f);;
	glBindBuffer(GL_ARRAY_BUFFER, _sn->bufferObjects[1]);	// NORMAL_DATA
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * tnVec.size(), &(tnVec[0]), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, _sn->bufferObjects[2]);	// TANGENT_DATA
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * tnVec.size(), &(tnVec[0]), GL_DYNAMIC_DRAW);
	// Texture coordinates
	glBindBuffer(GL_ARRAY_BUFFER, _sn->bufferObjects[3]);	// TEXTURE_DATA
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*_uv.size(), &(_uv[0]), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _sn->bufferObjects[4]);	// INDEX_DATA
	// Eliminate deleted triangles from viewing, but to keep the numbering send to graphics card
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*_tris.size(), &(_tris[0]), GL_STATIC_DRAW);
	getSkinIncisionLines();  // do this last as needs the vertex position buffer to paint incision lines.
}

void surgGraphics::getSkinIncisionLines() {
	_incisionLines.clear();  // indexes into incision lines. 0xffffffff is primitive restart index.
	std::map<int, int> triEdges, vTex;
	for (int n = _mt.numberOfTriangles(), i = 0; i < n; ++i) {
		int mat = _mt.triangleMaterial(i);
		if (mat < 0)
			continue;
		int at[3], ae[3];
		_mt.triangleAdjacencies(i, at, ae);  // triAdjs(i);
		const int* tr = _mt.triangleVertices(at[0]), * tx = _mt.triangleTextures(at[0]);
		if (mat == 3) {  // surface skin incision . 2-3 pair
			if (_mt.triangleMaterial(at[0]) != 2)  // incision convention
				continue;
			int first = tr[ae[0]], second = tr[(ae[0] + 1) % 3];
			triEdges.insert(std::make_pair(first, second));
			vTex.insert(std::make_pair(first, tx[ae[0]]));
		}
		if (mat == 6) {  // deep incision. 5-6, 6-7, or 6-8 pair
			for (int j = 0; j < 3; ++j) {
				int aMat = _mt.triangleMaterial(at[j]);
				if (aMat == 6 || aMat == 3)  // any different material except 3 which is a non-undermined deep cut
					continue;
				int first = tr[ae[j]], second = tr[(ae[j] + 1) % 3];
				triEdges.insert(std::make_pair(first, second));
				vTex.insert(std::make_pair(first, tx[ae[j]]));
			}
		}
	}
	std::map<int, int>::iterator eit;
	while ((eit = triEdges.begin()) != triEdges.end()) {
		int start = eit->first, second = eit->second;
		triEdges.erase(eit);
		auto next = triEdges.find(second);
		auto tex = vTex.find(start);
		if (tex == vTex.end())
			throw(std::logic_error("Program error in getSkinIncisionLines().\n"));
		_incisionLines.push_back(tex->second);
		while (next != triEdges.end() && next->second != start) {
			auto tex = vTex.find(next->first);
			if (tex == vTex.end())
				throw(std::logic_error("Program error in getSkinIncisionLines().\n"));
			_incisionLines.push_back(tex->second);
			second = next->second;
			triEdges.erase(next);
			next = triEdges.find(second);
		}
		if (next != triEdges.end()) {
			triEdges.erase(next);
			_incisionLines.push_back(vTex[start]);
		}
		_incisionLines.push_back(0xffffffff);
	}
	if (!_incisionLines.empty()) {
		if (!_incis.isInitialized()) {
			_incis.setGl3wGraphics(_gl3w);
			float color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
			_incis.setColor(color);
		}
		assert(_sn->bufferObjects[0] > 0);
		_incis.sendVertexCoordBuffer(_sn->bufferObjects[0]);
		_incis.addUpdateIncisions(_incisionLines);
	}
}

void surgGraphics::updatePositionsNormalsTangents()  // bool doTangents now always true
{
	for (int m = (int)_uvPos.size(), i = 0; i < m; ++i) {
		if (_uvPos[i] < 0)
			continue;
		float* fp = _mt.vertexCoordinate(_uvPos[i]);
		for (int j = 0; j < 3; ++j)
			_xyz1[(i << 2) + j] = fp[j];
	}
	std::vector<GLfloat> normals, tangents;
	normals.assign((_uv.size() >> 1) * 3, 0.0f);
	tangents.assign(normals.size(), 0.0f);
	int i=0,j,k,n;
	GLfloat *gv[3], *tv[3];
	n = (unsigned int)_tris.size();
	Vec3f nrmV,tanV,dXyz[2];
	float d2,dTx[2][2];
	for(i=0; i<n; i+=3) {
		if (_tris[i]>0xfffffffe)
			continue;
		for(j=0; j<3; ++j) {
			nrmV[j]=0.0f;
			gv[j] = &_xyz1[_tris[i+j] << 2];
			tv[j] = &_uv[_tris[i + j] << 1];
		}
		for(j=0; j<3; ++j) {
			dXyz[0][j] = gv[1][j]-gv[0][j];
			dXyz[1][j] = gv[2][j]-gv[0][j];
		}
		for (j = 0; j<2; ++j) {
			dTx[0][j] = tv[1][j] - tv[0][j];
			dTx[1][j] = tv[2][j] - tv[0][j];
		}
		d2 = dTx[0][0] * dTx[1][1] - dTx[1][0] * dTx[0][1];
		if (fabs(d2) < 1e-16f)
			tanV.set(0.0f, 0.0f, 0.0f);
		else
			tanV = (dXyz[0] * dTx[1][1] - dXyz[1] * dTx[0][1]) / d2;
		nrmV = dXyz[0] ^ dXyz[1];
		for (j = 0; j < 3; ++j) {
			k = _tris[i + j] * 3;
			normals[k] += nrmV[0];
			normals[k + 1] += nrmV[1];
			normals[k + 2] += nrmV[2];
			tangents[k] += tanV[0];
			tangents[k + 1] += tanV[1];
			tangents[k + 2] += tanV[2];
		}
	}
	for(auto &txs : _textureSeams){
		GLfloat ns[3] = { 0.0f, 0.0f, 0.0f }, ts[3] = { 0.0f, 0.0f, 0.0f };
		for (auto &bv : txs.second) {
			for (int j = 0; j < 3; ++j) {
				ns[j] += normals[bv * 3 + j];
				ts[j] += tangents[bv * 3 + j];
			}
		}
		for (auto& bv : txs.second) {
			for (int j = 0; j < 3; ++j) {
				normals[bv * 3 + j] = ns[j];
				tangents[bv * 3 + j] = ts[j];
			}
		}
	}
	auto invSqrt = [](float x) ->float{ // Steve Pizer's version of the Quake algorithm
		GLuint i = 0x5F1F1412 - (*(GLuint*)&x >> 1);
		float tmp = *(float*)&i;
		return tmp * (1.69000231f - 0.714158168f * x * tmp * tmp);
	};
	n = (int)normals.size();
	for (i = 0; i < n; i += 3) {
		if (_tris[i]>0xfffffffe)
			continue;
		d2 = normals[i] * normals[i] + normals[i + 1] * normals[i + 1] + normals[i + 2] * normals[i + 2];
		if (d2 < 1e-16f) {
			normals[i] = 0.0f; normals[i + 1] = 0.0f; normals[i + 2] = 1.0f;
		}
		else {
			d2 = invSqrt(d2);
			normals[i] *= d2; normals[i + 1] *= d2; normals[i + 2] *= d2;
		}
		d2 = tangents[i] * tangents[i] + tangents[i + 1] * tangents[i + 1] + tangents[i + 2] * tangents[i + 2];
		if (d2 < 1e-16f) {
			tangents[i] = 1.0f; tangents[i + 1] = 0.0f; tangents[i + 2] = 0.0f;
		}
		else {
			d2 = invSqrt(d2);
			tangents[i] *= d2; tangents[i + 1] *= d2; tangents[i + 2] *= d2;
		}
	}
	// Vertex data
	glBindBuffer(GL_ARRAY_BUFFER, _sn->bufferObjects[0]);	// VERTEX_DATA
	// now copy data into memory  glBufferSubdata() appears to be faster than memcopy into mapped buffer
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * _xyz1.size(), &(_xyz1[0]));
	glBindBuffer(GL_ARRAY_BUFFER, _sn->bufferObjects[1]);	// NORMAL_DATA
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * normals.size(), &(normals[0]));
	glBindBuffer(GL_ARRAY_BUFFER, _sn->bufferObjects[2]);	// TANGENT_DATA
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * tangents.size(), &(tangents[0]));
}

void surgGraphics::getTextureSeams() {
	// vertex positions with multiple textures of same material (2 or 5 guaranteed exclusive) associated with them for normal and tangent blending
//	_mt.findAdjacentTriangles(true);  // not necessary. done in calling routine
	auto addSeamVert = [&](int vPos, int tex0, int tex1) {
		auto pr = _textureSeams.insert(std::make_pair(vPos, std::list<int>()));
		if (tex0 > tex1) { int tmp = tex1; tex1 = tex0; tex0 = tmp; }
		if (pr.second) {
			pr.first->second.push_back(tex0);
			pr.first->second.push_back(tex1);
		}
		else {
			auto tit = pr.first->second.begin();
			while (tit != pr.first->second.end() && *tit < tex0)
				++tit;
			if(tit == pr.first->second.end() || *tit != tex0)
				tit = pr.first->second.insert(tit, tex0);
			while (tit != pr.first->second.end() && *tit < tex1)
				++tit;
			if (tit == pr.first->second.end() || *tit != tex1)
				tit = pr.first->second.insert(tit, tex1);
		}
	};
	for (int n = _mt.numberOfTriangles(), i = 0; i < n; ++i) {
		int at[3], ae[3], mat0 = _mt.triangleMaterial(i);
		if (mat0 != 2 && mat0 != 5)
			continue;
		_mt.triangleAdjacencies(i, at, ae);
		int *tx0 = _mt.triangleTextures(i);
		for (int j = 0; j < 3; ++j) {
			if (_mt.triangleMaterial(at[j]) != mat0)
				continue;
			int* tx1 = _mt.triangleTextures(at[j]);
			if (tx0[j] != tx1[(ae[j] + 1) % 3])
				addSeamVert(_mt.triangleVertices(i)[j], tx0[j], tx1[(ae[j] + 1) % 3]);
		}
	}
}

void surgGraphics::draw(void)
{
	glBindVertexArray(_sn->vertexArrayBufferObject);
	for (int n = (int)_sn->textureBuffers.size(), i = 0; i < n; ++i) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, _sn->textureBuffers[i]);
	}
	GLuint start = 0, end = 0;
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0, 1.0);
	int mat = -1, t=0, nTris = _mt.numberOfTriangles();
	while (t < nTris){
		int tMat = _mt.triangleMaterial(t);
		if (tMat < 0){  // deleted triangle
			end = t * 3;  // 3 indices per triangle
			glDrawElements(GL_TRIANGLES, (GLsizei)(end - start), GL_UNSIGNED_INT, (const GLvoid*)(sizeof(GLuint)*start));
			while (t<nTris && _mt.triangleMaterial(t) < 0)
				++t;
			start = t * 3;  // 3 indices per triangle
			continue;
		}
		if (tMat != mat){
			mat = tMat;
			_gl3w->getLightsShaders()->setMaterial(mat);
			start = t * 3;  // 3 indices per triangle
			++t;
		}
		while (t<nTris && mat == _mt.triangleMaterial(t))
			++t;
		end = t * 3;  // 3 indices per triangle
		glDrawElements(GL_TRIANGLES, (GLsizei)(end - start), GL_UNSIGNED_INT, (const GLvoid*)(sizeof(GLuint)*start));
	}
	glPolygonOffset(0.0, 0.0);
	glDisable(GL_POLYGON_OFFSET_FILL);

	// Never unbind a GL_ARRAY_BUFFER or GL_ELEMENT_ARRAY_BUFFER inside an active vertex array buffer object
	glBindVertexArray(0);
}

void surgGraphics::computeLocalBounds()
{
	if (_xyz1.empty())
		return;
	float minMaxXYZ[6];
	minMaxXYZ[0]=minMaxXYZ[2]=minMaxXYZ[4]=1e30f;
	minMaxXYZ[1]=minMaxXYZ[3]=minMaxXYZ[5]=-1e30f;
	int i,n=(unsigned int)_xyz1.size()>>2;
	GLfloat *v=&(_xyz1[0]);
	for(i=0; i<n; ++i)	{
		if(*v<minMaxXYZ[0])
			minMaxXYZ[0]=*v;
		if(*v>minMaxXYZ[1])
			minMaxXYZ[1]=*v;
		++v;
		if(*v<minMaxXYZ[2])
			minMaxXYZ[2]=*v;
		if(*v>minMaxXYZ[3])
			minMaxXYZ[3]=*v;
		++v;
		if(*v<minMaxXYZ[4])
			minMaxXYZ[4]=*v;
		if(*v>minMaxXYZ[5])
			minMaxXYZ[5]=*v;
		++v; ++v;
	}
	GLfloat lc[3], radius;
	lc[0] = (minMaxXYZ[0]+minMaxXYZ[1])*0.5f;
	lc[1] = (minMaxXYZ[2]+minMaxXYZ[3])*0.5f;
	lc[2] = (minMaxXYZ[4]+minMaxXYZ[5])*0.5f;
	radius = (lc[0]-minMaxXYZ[0])*(lc[0]-minMaxXYZ[0]);
	radius += (lc[1]-minMaxXYZ[2])*(lc[1]-minMaxXYZ[2]);
	radius += (lc[2]-minMaxXYZ[4])*(lc[2]-minMaxXYZ[4]);
	radius = sqrt(radius);
	_sn->setLocalBounds(lc, radius);
}

surgGraphics::surgGraphics() : _undermineTriangles(NULL), _sn(nullptr)
{
	_incis.setSurgGraphics(this);
}

surgGraphics::~surgGraphics(void)
{
	if (!_sn)
		return;
	if (!_sn->bufferObjects.empty()) {
		glDeleteBuffers((GLsizei)_sn->bufferObjects.size(), &_sn->bufferObjects[0]);
		_sn->bufferObjects.clear();
	}
	if (_sn->vertexArrayBufferObject > 0) {
		glDeleteVertexArrays(1, &_sn->vertexArrayBufferObject);
		_sn->vertexArrayBufferObject = 0;
	}
	if (!_sn->textureBuffers.empty())
		glDeleteTextures((GLsizei)_sn->textureBuffers.size(), &(_sn->textureBuffers[0]));

}

void incisionLines::addUpdateIncisions(const std::vector<GLuint> &lines){
	if (!_isn) {
		_isn = std::make_shared<sceneNode>();
		_isn->setGl3wGraphics(_gl3w);
		_isn->setName("incisionLines");
		_isn->setType(sceneNode::nodeType::LINES);
		GLfloat* mm = _isn->getModelViewMatrix();
		loadIdentity4x4(mm);
		GLuint program = _gl3w->getLightsShaders()->getOrCreateLineProgram();
		_isn->setGlslProgramNumber(program);
		_isn->setColorLocation(glGetUniformLocation(program, "objectColor"));
		GLfloat color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		_isn->setColor(color);
		if (_incisionVertexArrayBufferObject == 0xffffffff)
			glGenVertexArrays(1, &_incisionVertexArrayBufferObject);
		_isn->vertexArrayBufferObject = _incisionVertexArrayBufferObject;
		if (_incisionBufferObjects[0] == 0xffffffff) {
			glGenBuffers(1, &_incisionBufferObjects[0]);
		}
		_isn->bufferObjects.assign(2, 0);
		_isn->bufferObjects[0] = _incisionBufferObjects[0];
		_isn->bufferObjects[1] = _incisionBufferObjects[1];  // _xyz1 data sent from surgGraphics
	// Create the master vertex array object
		glBindVertexArray(_incisionVertexArrayBufferObject);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _isn->bufferObjects[0]);
		// Vertex data
		glBindBuffer(GL_ARRAY_BUFFER, _isn->bufferObjects[1]);	// VERTEX DATA
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
		// Unbind to anybody
		glBindVertexArray(0);
		_gl3w->addSceneNode(_isn);
	}
	_isn->setColor(_color);
	// 0xffffffff is primitive restart index
	// vertex xyz1 buffer data comes from the surgGraphics data already loaded
	// Indexes
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _isn->bufferObjects[0]);	// INDEX_DATA
	_isn->elementArraySize = (GLsizei)(sizeof(GLuint) * lines.size());
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, _isn->elementArraySize, &(lines[0]), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


