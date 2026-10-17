#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <rw.h>
#include <skeleton.h>
#include <args.h>

char *argv0;

rw::EngineOpenParams engineOpenParams;

rw::RGBA BackgroundColor = { 64, 64, 96, 255 };

// defaults, so the ps2 build has something to load without arguments
static const char *dffPath = "files/world.dff";
static const char *txdPath = nil;
static const char *imagePath = "files/";

struct SceneGlobals
{
	rw::World *world;
	rw::Camera *camera;
	rw::Clump *clump;
};
static SceneGlobals Scene;

static sk::Camera Cam;
static int flyMode = 0;
static float TimeDelta;

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-i imagepath] file.dff [file.txd]\n", argv0);
	exit(1);
}

static void
ParseArgs(void)
{
	int argc = sk::args.argc;
	char **argv = sk::args.argv;

	ARGBEGIN{
	case 'i':
		imagePath = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND;

	// an explicit dff drops the default txd, it wouldn't belong to it
	if(argc > 0){
		dffPath = argv[0];
		txdPath = nil;
	}
	if(argc > 1)
		txdPath = argv[1];
}

/*
 * Loading.
 * Read whole files and stream from memory: seeking on the ps2 host
 * fs is slow and negative seeks over ps2link are broken.
 */

static void*
ReadFile(const char *path, rw::uint32 *len)
{
	char buf[SK_MAXPATH];
	void *data;

	data = rw::getFileContents(sk::GetFilePath(buf, path), len);
	if(data == nil)
		printf("couldn't open %s\n", buf);
	return data;
}

static rw::TexDictionary*
LoadTexDictionary(const char *path)
{
	rw::StreamMemory in;
	rw::TexDictionary *txd;
	rw::uint8 *data;
	rw::uint32 len;

	data = (rw::uint8*)ReadFile(path, &len);
	if(data == nil)
		return nil;
	txd = nil;
	in.open(data, len);
	if(rw::findChunk(&in, rw::ID_TEXDICTIONARY, nil, nil))
		txd = rw::TexDictionary::streamRead(&in);
	in.close();
	rwFree(data);
	return txd;
}

static rw::Clump*
LoadClump(const char *path)
{
	rw::StreamMemory in;
	rw::Clump *clump;
	rw::uint8 *data;
	rw::uint32 len;

	data = (rw::uint8*)ReadFile(path, &len);
	if(data == nil)
		return nil;
	clump = nil;
	in.open(data, len);
	if(rw::findChunk(&in, rw::ID_CLUMP, nil, nil))
		clump = rw::Clump::streamRead(&in);
	in.close();
	rwFree(data);
	return clump;
}

/*
 * Fit the camera to whatever we loaded, so any model shows up
 * at a sensible size without fiddling.
 */
static void
FrameClump(rw::Clump *clump)
{
	rw::Sphere *sph;
	rw::V3d center, d;
	float radius, dist;
	int n;

	center.set(0.0f, 0.0f, 0.0f);
	n = 0;

	{
		FORLIST(lnk, clump->atomics){
			sph = rw::Atomic::fromClump(lnk)->getWorldBoundingSphere();
			center = rw::add(center, sph->center);
			n++;
		}
	}
	if(n == 0)
		return;
	center = rw::scale(center, 1.0f/n);

	// radius that covers every atomic's sphere from that center
	radius = 0.0f;
	{
		FORLIST(lnk, clump->atomics){
			sph = rw::Atomic::fromClump(lnk)->getWorldBoundingSphere();
			dist = rw::length(rw::sub(sph->center, center)) + sph->radius;
			if(dist > radius)
				radius = dist;
		}
	}
	if(radius < 0.001f)
		radius = 1.0f;

	d.set(0.0f, -radius*2.5f, radius*0.7f);
	Cam.m_target = center;
	Cam.m_position = rw::add(center, d);
	Cam.m_near = radius*0.01f;
	Cam.m_far = radius*20.0f;
	if(Cam.m_near < 0.05f) Cam.m_near = 0.05f;
	if(Cam.m_far < 100.0f) Cam.m_far = 100.0f;

	// so flying feels the same on a teapot and on a world
	sk::cameraControls.moveSpeed = radius;
}

void
Initialize(void)
{
	sk::globals.windowtitle = "librw clump view";
	sk::globals.width = 1280;
	sk::globals.height = 800;
	sk::globals.quit = 0;
}

bool
Initialize3D(void)
{
	char buf[SK_MAXPATH];
	rw::TexDictionary *txd;
	rw::Light *light;
	rw::V3d xaxis = { 1.0f, 0.0f, 0.0f };

	if(!sk::InitRW())
		return false;

	rw::Image::setSearchPath(sk::GetFilePath(buf, imagePath));

	if(txdPath){
		txd = LoadTexDictionary(txdPath);
		if(txd)
			rw::TexDictionary::setCurrent(txd);
	}

	Scene.clump = LoadClump(dffPath);
	if(Scene.clump == nil)
		return false;

	Scene.world = rw::World::create();

	light = rw::Light::create(rw::Light::AMBIENT);
	light->setColor(0.4f, 0.4f, 0.4f);
	Scene.world->addLight(light);

	light = rw::Light::create(rw::Light::DIRECTIONAL);
	light->setColor(0.7f, 0.7f, 0.7f);
	light->setFrame(rw::Frame::create());
	light->getFrame()->rotate(&xaxis, 150.0f, rw::COMBINEREPLACE);
	Scene.world->addLight(light);

	Scene.world->addClump(Scene.clump);

	Scene.camera = sk::CameraCreate(sk::globals.width, sk::globals.height, 1);
	assert(Scene.camera);
	Scene.world->addCamera(Scene.camera);

	Cam.init();
	Cam.attach(Scene.camera);
	Cam.m_aspectRatio = (float)sk::globals.width/sk::globals.height;
	FrameClump(Scene.clump);
	Cam.update();

#ifndef RW_PS2
	ImGui_ImplRW_Init();
	ImGui::StyleColorsClassic();
#endif

	return true;
}

void
Terminate3D(void)
{
	if(Scene.clump){
		Scene.world->removeClump(Scene.clump);
		Scene.clump->destroy();
		Scene.clump = nil;
	}
	if(Scene.camera){
		Scene.world->removeCamera(Scene.camera);
		sk::CameraDestroy(Scene.camera);
		Scene.camera = nil;
	}
	// TODO: lights?
	if(Scene.world){
		Scene.world->destroy();
		Scene.world = nil;
	}
	sk::TerminateRW();
}

bool
attachPlugins(void)
{
	rw::ps2::registerPDSPlugin(40);
	rw::ps2::registerPluginPDSPipes();

	rw::registerMeshPlugin();
	rw::registerNativeDataPlugin();
	rw::registerAtomicRightsPlugin();
	rw::registerMaterialRightsPlugin();
	rw::xbox::registerVertexFormatPlugin();
	rw::registerSkinPlugin();
	rw::registerUserDataPlugin();
	rw::registerHAnimPlugin();
	rw::registerMatFXPlugin();
	rw::registerUVAnimPlugin();
	rw::ps2::registerADCPlugin();
	return true;
}

#ifndef RW_PS2
void
Gui(void)
{
	static bool showWindow = true;

	ImGui::Begin("clump view", &showWindow);
	ImGui::Text("%s", dffPath);
	ImGui::Text("%.1f fps", 1.0f/TimeDelta);
	ImGui::NewLine();
	if(ImGui::RadioButton("fly", flyMode != 0))
		flyMode = 1;
	if(ImGui::RadioButton("editor", flyMode == 0))
		flyMode = 0;
	ImGui::NewLine();
	ImGui::DragFloat("move speed", &sk::cameraControls.moveSpeed, 0.1f, 0.01f, 1e6f);
	ImGui::DragFloat("mouse sens", &sk::cameraControls.mouseSens, 0.1f, 0.01f, 100.0f);
	ImGui::End();
}
#endif

void
Render(void)
{
	Scene.camera->clear(&BackgroundColor, rw::Camera::CLEARIMAGE|rw::Camera::CLEARZ);
	Scene.camera->beginUpdate();

#ifndef RW_PS2
	ImGui_ImplRW_NewFrame(TimeDelta);
#endif

	Scene.world->render();

#ifndef RW_PS2
	Gui();
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplRW_RenderDrawLists(ImGui::GetDrawData());
#endif

	Scene.camera->endUpdate();
	Scene.camera->showRaster(0);
}

void
Idle(float timeDelta)
{
	int wantmouse;

	TimeDelta = timeDelta;
	if(TimeDelta <= 0.0f) TimeDelta = 1.0f/60.0f;
	if(TimeDelta > 0.1f) TimeDelta = 0.1f;

	wantmouse = 0;
#ifndef RW_PS2
	wantmouse = ImGui::GetIO().WantCaptureMouse;
#endif
	if(!wantmouse){
		if(flyMode)
			sk::CameraFlyControls(&Cam, TimeDelta);
		else
			sk::CameraEditorControls(&Cam, TimeDelta);
	}
	Cam.update();

	Render();
}

void
KeyDown(int key)
{
	switch(key){
	case sk::KEY_ESC:
		sk::globals.quit = 1;
		break;
	case sk::KEY_TAB:
		flyMode = !flyMode;
		break;
	case 'F':
		FrameClump(Scene.clump);
		break;
	}
}

sk::EventStatus
AppEventHandler(sk::Event e, void *param)
{
	using namespace sk;
	Rect *r;

#ifndef RW_PS2
	ImGuiEventHandler(e, param);
#endif

	switch(e){
	case INITIALIZE:
		ParseArgs();
		Initialize();
		return EVENTPROCESSED;
	case RWINITIALIZE:
		return Initialize3D() ? EVENTPROCESSED : EVENTERROR;
	case RWTERMINATE:
		Terminate3D();
		return EVENTPROCESSED;
	case PLUGINATTACH:
		return attachPlugins() ? EVENTPROCESSED : EVENTERROR;
	case KEYDOWN:
		KeyDown(*(int*)param);
		return EVENTPROCESSED;
	case RESIZE:
		r = (Rect*)param;
		if(r->w == 0) r->w = 1;
		if(r->h == 0) r->h = 1;
		sk::globals.width = r->w;
		sk::globals.height = r->h;
		if(Scene.camera){
			sk::CameraSize(Scene.camera, r, 0.5f, 4.0f/3.0f);
			Cam.m_aspectRatio = (float)r->w/r->h;
		}
		break;
	case IDLE:
		Idle(*(float*)param);
		return EVENTPROCESSED;
	}
	return sk::EVENTNOTPROCESSED;
}
