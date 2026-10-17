#include <math.h>

#include <rw.h>
#include "skeleton.h"

#define PI 3.14159265359f

namespace sk {

using rw::Quat;
using rw::V3d;

void
Camera::init(void)
{
	m_position.set(0.0f, 6.0f, 0.0f);
	m_target.set(0.0f, 0.0f, 0.0f);
	m_up.set(0.0f, 0.0f, 1.0f);
	m_localup = m_up;
	m_fov = 70.0f;
	m_aspectRatio = 1.0f;
	m_near = 0.1f;
	m_far = 100.0f;
	m_rwcam = nil;
}

void
Camera::attach(rw::Camera *cam)
{
	m_rwcam = cam;
}

void
Camera::update(void)
{
	rw::Frame *f;
	V3d forward, left, nup;

	if(m_rwcam == nil)
		return;
	m_rwcam->setNearPlane(m_near);
	m_rwcam->setFarPlane(m_far);
	m_rwcam->setFOV(m_fov, m_aspectRatio);

	f = m_rwcam->getFrame();
	if(f == nil)
		return;
	forward = normalize(sub(m_target, m_position));
	left = normalize(cross(m_up, forward));
	nup = cross(forward, left);
	f->matrix.right = left;	// lol
	f->matrix.up = nup;
	f->matrix.at = forward;
	f->matrix.pos = m_position;
	f->matrix.optimize();
	f->updateObjects();
}

void
Camera::setTarget(V3d target)
{
	m_position = sub(m_position, sub(m_target, target));
	m_target = target;
}

float
Camera::getHeading(void)
{
	V3d dir = sub(m_target, m_position);
	float a = atan2(dir.y, dir.x) - PI/2.0f;
	return m_localup.z < 0.0f ? a-PI : a;
}

void
Camera::turn(float yaw, float pitch)
{
	V3d dir, right;
	Quat r;

	dir = sub(m_target, m_position);
	r = Quat::rotation(yaw, rw::makeV3d(0.0f, 0.0f, 1.0f));
	dir = rotate(dir, r);
	m_localup = rotate(m_localup, r);

	right = normalize(cross(dir, m_localup));
	r = Quat::rotation(pitch, right);
	dir = rotate(dir, r);
	m_localup = normalize(cross(right, dir));
	if(m_localup.z >= 0.0f) m_up.z = 1.0f;
	else m_up.z = -1.0f;

	m_target = add(m_position, dir);
}

void
Camera::orbit(float yaw, float pitch)
{
	V3d dir, right;
	Quat r;

	dir = sub(m_target, m_position);
	r = Quat::rotation(yaw, rw::makeV3d(0.0f, 0.0f, 1.0f));
	dir = rotate(dir, r);
	m_localup = rotate(m_localup, r);

	right = normalize(cross(dir, m_localup));
	r = Quat::rotation(-pitch, right);
	dir = rotate(dir, r);
	m_localup = normalize(cross(right, dir));
	if(m_localup.z >= 0.0f) m_up.z = 1.0f;
	else m_up.z = -1.0f;

	m_position = sub(m_target, dir);
}

void
Camera::dolly(float dist)
{
	V3d dir = setlength(sub(m_target, m_position), dist);
	m_position = add(m_position, dir);
	m_target = add(m_target, dir);
}

void
Camera::zoom(float dist)
{
	V3d dir = sub(m_target, m_position);
	float curdist = length(dir);
	if(dist >= curdist)
		dist = curdist - 0.01f;
	dir = setlength(dir, dist);
	m_position = add(m_position, dir);
}

void
Camera::pan(float x, float y)
{
	V3d dir = normalize(sub(m_target, m_position));
	V3d right = normalize(cross(dir, m_up));
	V3d localup = normalize(cross(right, dir));
	dir = add(scale(right, x), scale(localup, y));
	m_position = add(m_position, dir);
	m_target = add(m_target, dir);
}

float
Camera::distanceTo(V3d v)
{
	return length(sub(m_position, v));
}

float
Camera::distanceToTarget(void)
{
	return length(sub(m_position, m_target));
}

rw::bool32
Camera::isSphereVisible(rw::Sphere *sph, rw::Matrix *xform)
{
	rw::Sphere sphere = *sph;
	if(xform)
		rw::V3d::transformPoints(&sphere.center, &sphere.center, 1, xform);
	return m_rwcam->frustumTestSphere(&sphere) != rw::Camera::SPHEREOUTSIDE;
}

/*
 * Controls
 */

CameraControls cameraControls;

void
CameraControlsInit(void)
{
	cameraControls.mouseSens = 4.5f;
	cameraControls.wheelSens = 0.2f;
	cameraControls.stickSens = 2.0f;
	cameraControls.moveSpeed = 30.0f;
	cameraControls.speedUpMult = 4.0f;
}

// Mouse motion since the last frame, normalized to the viewport.
// This is a displacement, so it must NOT be scaled by dt.
static void
MouseDelta(float *dx, float *dy)
{
	*dx = (float)(mouse.posx - prevmouse.posx) / (float)globals.width;
	*dy = (float)(mouse.posy - prevmouse.posy) / (float)globals.height;
}

// Move along world up, keeping the view direction.
static void
MoveUp(Camera *cam, float dist)
{
	V3d d = scale(cam->m_up, dist);
	cam->m_position = add(cam->m_position, d);
	cam->m_target = add(cam->m_target, d);
}

void
CameraEditorControls(Camera *cam, float dt)
{
	float dx, dy, s, d;

	MouseDelta(&dx, &dy);
	s = cameraControls.mouseSens;
	d = cam->distanceToTarget();

	if(mouse.buttons & 1)
		cam->orbit(-dx*s, dy*s);
	else if(mouse.buttons & 2)
		cam->pan(-dx*d, dy*d);
	else if(mouse.buttons & 4)
		cam->zoom(-dy*s*d);
	// zoom proportional to distance so it approaches the target smoothly
	if(mouse.wheelDelta != 0.0f)
		cam->zoom(mouse.wheelDelta * cameraControls.wheelSens * d);

	// stick deflection is a rate
	if(pad.connected){
		cam->orbit(-pad.leftx*cameraControls.stickSens*dt,
		           -pad.lefty*cameraControls.stickSens*dt);
		cam->pan(-pad.rightx*d*dt, pad.righty*d*dt);
		if(pad.buttons & PADL1)
			cam->zoom(-d*dt);
		if(pad.buttons & PADR1)
			cam->zoom(d*dt);
	}
}

void
CameraFlyControls(Camera *cam, float dt)
{
	float dx, dy, s, speed;

	MouseDelta(&dx, &dy);
	s = cameraControls.mouseSens;

	if(mouse.buttons & 1)
		cam->turn(-dx*s, -dy*s);

	// stick look is a rate
	if(pad.connected)
		cam->turn(-pad.rightx*cameraControls.stickSens*dt,
		          -pad.righty*cameraControls.stickSens*dt);

	speed = cameraControls.moveSpeed;
	if(keys[KEY_LSHIFT] || keys[KEY_RSHIFT] || (pad.buttons & PADR1))
		speed *= cameraControls.speedUpMult;
	speed *= dt;

	if(keys['W']) cam->dolly(speed);
	if(keys['S']) cam->dolly(-speed);
	if(keys['A']) cam->pan(-speed, 0.0f);
	if(keys['D']) cam->pan(speed, 0.0f);
	if(keys['Q']) MoveUp(cam, -speed);
	if(keys['E']) MoveUp(cam, speed);

	if(pad.connected){
		cam->dolly(-pad.lefty*speed);
		cam->pan(pad.leftx*speed, 0.0f);
		if(pad.buttons & PADL2) MoveUp(cam, -speed);
		if(pad.buttons & PADR2) MoveUp(cam, speed);
	}
}

}
