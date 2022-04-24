#include "Poco/Util/Application.h"
#include "Poco/Util/Subsystem.h"
#include "Poco/Logger.h"

#include "PxPhysicsAPI.h"
#include "PxSimpleFactory.h"
#include "task/PxCpuDispatcher.h"
#include "extensions/PxDefaultAllocator.h"

using Poco::Util::Subsystem;
using namespace physx;


#define PVD_HOST "192.168.178.144"

// Physx Globals

class physx_subsystem: public Subsystem, public physx::PxCpuDispatcher
{
public:
	physx_subsystem(void);
	~physx_subsystem(void);
	
	uint8_t newScene(std::string sceneName);
	Poco::Util::Application *self_app;
 	void 	submitTask (PxBaseTask &task);
 	uint32_t getWorkerCount	()	const;

protected:
	void initialize(Poco::Util::Application &app);
	void reinitialize(Poco::Util::Application &app);
	void uninitialize();

	void defineOptions(Poco::Util::OptionSet& options);
	virtual const char* name() const;
private:


};
