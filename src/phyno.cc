#include <iostream>
#include <cstring>

#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/IniFileConfiguration.h"
#include "Poco/Logger.h"
#include "Poco/AsyncChannel.h"
#include "Poco/ConsoleChannel.h"
#include "Poco/AutoPtr.h"

#include "phynoMqttSubsystem.h"
#include "phynoPhysxSubsystem.h"

using Poco::Logger;
using Poco::AsyncChannel;
using Poco::ConsoleChannel;
using Poco::AutoPtr;
using Poco::Util::IniFileConfiguration;

class phyno: public Poco::Util::ServerApplication
{
protected:
   Logger* logger;

public:
   phyno() {}
   ~phyno() {}

protected:
   void initialize(Application& self)
   {
      setup_logging_channel();
      logger->information("Phyno is starting...");   
      loadConfiguration(); // load default configuration files, if present
      addSubsystem(new mqtt_subsystem);
      addSubsystem(new physx_subsystem);
      ServerApplication::initialize(self);
   }

   void uninitialize() { ServerApplication::uninitialize(); }

   void handleOption(const std::string& name, const std::string& value)
   {
      logger->information("Phyno have found parameter...");
      ServerApplication::handleOption(name, value);

   }

   int main(const std::vector<std::string>& /*args*/)
   {
      waitForTerminationRequest();
      return Application::EXIT_OK;
   }
private:
   int setup_logging_channel()
      {
      AutoPtr<ConsoleChannel> pCons(new ConsoleChannel);
      AutoPtr<AsyncChannel> pAsync(new AsyncChannel(pCons));
      Logger::root().setChannel(pAsync);
      logger = &Logger::get("PhynoMainLogger");
      return(0);
      }

};

int main(int argc, char** argv)
{
   phyno app;
   return app.run(argc, argv);
}