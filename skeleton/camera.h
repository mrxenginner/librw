#ifndef SKELCAMERA_H
#define SKELCAMERA_H

namespace sk {

// Manipulator around an rw::Camera. Keeps position/target/up and
// drives the rw camera's frame from them in update().
struct Camera
{
	rw::Camera *m_rwcam;
	rw::V3d m_position;
	rw::V3d m_target;
	rw::V3d m_up;
	rw::V3d m_localup;

	float m_fov, m_aspectRatio;
	float m_near, m_far;

	void init(void);
	void attach(rw::Camera *cam);

	void setTarget(rw::V3d target);
	float getHeading(void);

	void turn(float yaw, float pitch);
	void orbit(float yaw, float pitch);
	void dolly(float dist);
	void zoom(float dist);
	void pan(float x, float y);

	void update(void);
	float distanceTo(rw::V3d v);
	float distanceToTarget(void);
	rw::bool32 isSphereVisible(rw::Sphere *sph, rw::Matrix *xform);
};

/*
 * Controls.
 *
 * Two kinds of input, and they must not be treated alike:
 *
 *  displacement - mouse motion, wheel. Already frame rate independent,
 *                 a given hand movement gives the same delta at any fps.
 *                 Scale by a constant gain, never by dt.
 *  rate         - held keys, stick deflection. A speed, so scale by dt.
 *
 * Mouse deltas are normalized by the viewport, so sensitivity does not
 * change with resolution.
 */
struct CameraControls
{
	float mouseSens;	// gain for mouse displacement
	float wheelSens;	// gain for wheel displacement
	float stickSens;	// radians/sec for stick look
	float moveSpeed;	// units/sec
	float speedUpMult;	// while shift/R1 held
};
extern CameraControls cameraControls;

void CameraControlsInit(void);

// Editor style: orbit/pan/zoom about the target.
// mouse: left orbit, middle pan, right zoom, wheel zoom
// pad:   left stick orbit, right stick pan, L1/R1 zoom
void CameraEditorControls(Camera *cam, float dt);

// First person style: look with the mouse/right stick, fly with the keys.
// mouse: left drag look
// keys:  WASD move, Q/E down/up, shift faster
// pad:   right stick look, left stick move, L2/R2 down/up, R1 faster
void CameraFlyControls(Camera *cam, float dt);

}

#endif
