#include "graphics/al_Config.h"
#include "graphics/al_Light.hpp"

namespace al {
namespace gfx{

Material::Material(Face::t f)
:	mAmbient(0), mDiffuse(1), mEmission(0), mSpecular(0),
	mShine(0), mFace(f)
{
	
}
void Material::operator()() const {
	glMaterialfv(mFace, GL_AMBIENT, mAmbient.components);
	glMaterialfv(mFace, GL_DIFFUSE, mDiffuse.components);
	glMaterialfv(mFace, GL_EMISSION, mEmission.components);
	glMaterialfv(mFace, GL_SPECULAR, mSpecular.components);
	glMaterialf(mFace, GL_SHININESS, mShine);
}
Material& Material::ambientAndDiffuse(const Color& v){ ambient(v); return diffuse(v); }
Material& Material::ambient(const Color& v){ mAmbient=v; return *this; }
Material& Material::diffuse(const Color& v){ mDiffuse=v; return *this; }
Material& Material::emission(const Color& v){ mEmission=v; return *this; }
Material& Material::specular(const Color& v){ mSpecular=v; return *this; }
Material& Material::shininess(float v){ mShine=v; return *this; }
Material& Material::face(Face::t f){ mFace=f; return *this; }

static bool * lightPool(){
	static bool x[8] = {0};
	return x;
}

static int lightID(int i){
	static int x[]={GL_LIGHT0,GL_LIGHT1,GL_LIGHT2,GL_LIGHT3,GL_LIGHT4,GL_LIGHT5,GL_LIGHT6,GL_LIGHT7};
	return x[i];
}

static int nextID(){
	for(int i=0; i<8; ++i){
		if(!lightPool()[i]){
			lightPool()[i]=true;
			return i;
		}
	}
	return 7;
}

static void freeID(int i){ lightPool()[i]=false; }


Light::Light(float x, float y, float z)
:	mID(nextID()), mAmbient(0.2), mDiffuse(0.7), mSpecular(1)
{
	mPos[3]=1;
	pos(x,y,z);
	attenuation(1,0,0);
}

Light::~Light(){
	freeID(mID);
}

void Light::operator()() const {
	glEnable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);
	int glID = lightID(mID);
	glLightfv(glID, GL_AMBIENT,		mAmbient.components);
	glLightfv(glID, GL_DIFFUSE,		mDiffuse.components);
	glLightfv(glID, GL_SPECULAR,	mSpecular.components);
	glLightfv(glID, GL_POSITION,	mPos);
    glLightf(glID, GL_CONSTANT_ATTENUATION,	mAtten[0]);
    glLightf(glID, GL_LINEAR_ATTENUATION,	mAtten[1]);
    glLightf(glID, GL_QUADRATIC_ATTENUATION,mAtten[2]);
	
	glEnable(glID); // MUST enable each light source after configuration
	//glShadeModel(GL_FLAT);
}

Light& Light::attenuation(float c0, float c1, float c2){
	mAtten[0]=c0; mAtten[1]=c1; mAtten[2]=c2; return *this;
}

Light& Light::ambient(const Color& v){ mAmbient=v; return *this; }
Light& Light::diffuse(const Color& v){ mDiffuse=v; return *this; }
Light& Light::specular(const Color& v){ mSpecular=v; return *this; }

Light& Light::dir(float x, float y, float z){
	mPos[0]=x; mPos[1]=y; mPos[2]=z; mPos[3]=0;
	return *this;
}

Light& Light::pos(float x, float y, float z){
	mPos[0]=x; mPos[1]=y; mPos[2]=z; mPos[3]=1;
	return *this;
}

void Light::localViewer(bool v){
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, v ? GL_TRUE : GL_FALSE);
}

void Light::twoSided(bool v){
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, v ? GL_TRUE : GL_FALSE);
}

} // ::al::gfx
} // ::al
