#include "pch.h"
#include "TooFast.h"


BAKKESMOD_PLUGIN(TooFast, "Too Fast", plugin_version, PLUGINTYPE_THREADED)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;
bool enableSpeedMultiplier = false;
float speedMultiplier = 1.01;
float speedThreshold = 2300.0;
float maxSpeed = 2300.0;

void TooFast::onLoad()
{
	_globalCvarManager = cvarManager;
	//cvarManager->log("Plugin loaded!");

	//cvarManager->registerNotifier("my_aweseome_notifier", [&](std::vector<std::string> args) {
	//	cvarManager->log("Hello notifier!");
	//}, "", 0);

	//auto cvar = cvarManager->registerCvar("template_cvar", "hello-cvar", "just a example of a cvar");
	//auto cvar2 = cvarManager->registerCvar("template_cvar2", "0", "just a example of a cvar with more settings", true, true, -10, true, 10 );

	//cvar.addOnValueChanged([this](std::string cvarName, CVarWrapper newCvar) {
	//	cvarManager->log("the cvar with name: " + cvarName + " changed");
	//	cvarManager->log("the new value is:" + newCvar.getStringValue());
	//});

	//cvar2.addOnValueChanged(std::bind(&TooFast::YourPluginMethod, this, _1, _2));

	// enabled decleared in the header
	//enabled = std::make_shared<bool>(false);
	cvarManager->registerCvar("too_fast_enabled", "0", "Enable the Too Fast plugin", true, true, 0, true, 1)
		.addOnValueChanged([this](std::string, CVarWrapper cvar) {
		if (cvar.getBoolValue()) {
			hookEvents();
		} else {
			unhookEvents();
		}
		});

	cvarManager->registerCvar("too_fast_speed_multiplier_enabled", "0", "Enable the speed multiplier", true, true, 0, true, 1)
		.addOnValueChanged([this](std::string, CVarWrapper cvar) {
			enableSpeedMultiplier = cvar.getBoolValue();
			});

	cvarManager->registerCvar("too_fast_speed_multiplier", "1.01", 
			"Multiplies speed by this amount each tick", true, true, 1, true, 2)
		.addOnValueChanged([this](std::string, CVarWrapper cvar) { speedMultiplier = cvar.getFloatValue(); });

	cvarManager->registerCvar("too_fast_speed_threshold", "2300", "Speed where the plugin stops increasing acceleration (negative to disable)", true, true, -1, false, 2300)
		.addOnValueChanged([this](std::string, CVarWrapper cvar) { speedThreshold = cvar.getFloatValue(); });
	
	cvarManager->registerCvar("too_fast_max_speed", "2300", "Sets new max speed for the main player", true, true, 0, false, 2300)
		.addOnValueChanged([this](std::string, CVarWrapper cvar) { maxSpeed = cvar.getFloatValue(); });

	//cvarManager->registerNotifier("too_fast_double_max_speed", [this](std::vector<std::string> params){ 
	//	CarWrapper myCar = gameWrapper->GetLocalCar();

	//	if (myCar.IsNull()) {
	//		return;
	//	}
	//	//float maxAngularSpeed = myCar.GetMaxAngularSpeed();
	//	//myCar.SetMaxAngularSpeed2(maxAngularSpeed * 2);
	//	
	//	cvarManager->log("new max speed " + std::to_string(myCar.GetMaxLinearSpeed()));
	//	}, "Increases car's max speed", PERMISSION_ALL);
	
	cvarManager->registerNotifier("too_fast_get_max_speed", [this](std::vector<std::string> params) {
		CarWrapper myCar = gameWrapper->GetLocalCar();

		if (myCar.IsNull()) {
			return;
		}
		cvarManager->log("max speed " + std::to_string(myCar.GetMaxLinearSpeed()));
		}, "Increases car's max speed", PERMISSION_ALL);

	/*cvarManager->registerNotifier("too_fast_explode", [this](std::vector<std::string> params) {
		CarWrapper myCar = gameWrapper->GetLocalCar();

		if (myCar.IsNull()) {
			return;
		}

		myCar.Demolish();
		
		}, "Increases car's max speed", PERMISSION_ALL);*/
	//cvarManager->registerCvar("CVAR", "DEFAULTVALUE", "DESCRIPTION", true, true, MINVAL, true, MAXVAL);//.bindTo(CVARVARIABLE);
	//gameWrapper->HookEvent("FUNCTIONNAME", std::bind(&TEMPLATE::FUNCTION, this));
	//gameWrapper->HookEventWithCallerPost<ActorWrapper>("FUNCTIONNAME", std::bind(&TooFast::FUNCTION, this, _1, _2, _3));
	//gameWrapper->RegisterDrawable(bind(&TEMPLATE::Render, this, std::placeholders::_1));


	//gameWrapper->HookEvent("Function TAGame.Ball_TA.Explode", [this](std::string eventName) {
	//	cvarManager->log("Your hook got called and the ball went POOF");
	//});
	// You could also use std::bind here
	//gameWrapper->HookEvent("Function TAGame.Ball_TA.Explode", std::bind(&TooFast::YourPluginMethod, this);
}

void TooFast::onUnload()
{
}

// hooks events to allow the plugin to work
void TooFast::hookEvents() {
	gameWrapper->HookEventWithCallerPost<CarWrapper>("Function TAGame.Car_TA.SetVehicleInput",
		[this](CarWrapper caller, void*, std::string) { onTick(caller); });
}

// hooks events to allow the plugin to work
void TooFast::unhookEvents() {
	gameWrapper->UnhookEventPost("Function TAGame.Car_TA.SetVehicleInput");
}

ServerWrapper TooFast::getSW() {
	if (gameWrapper->IsInOnlineGame()) {
		cvarManager->log("null server");
		return NULL;
	}
	if (gameWrapper->IsInGame()) {
		auto server = gameWrapper->GetGameEventAsServer();

		if (server.IsNull()) {
			cvarManager->log("null server");
			return NULL;
		}

		return server;
	}
	//cvarManager->log("no server");
	return NULL;
}

void TooFast::onTick(CarWrapper caller) {
	auto sw = getSW();

	if (sw.IsNull()) return;

	CarWrapper myCar = gameWrapper->GetLocalCar();

	if (myCar.IsNull()) {
		return;
	}

	if (caller.memory_address != myCar.memory_address) {
		return;
	}

	// mex speed is reset when demoed, so ensures it's always valid
	myCar.SetMaxLinearSpeed2(maxSpeed);

	auto controller = myCar.GetPlayerController();

	auto input = controller.GetVehicleInput();
	//cvarManager->log("Throttle : " + std::to_string(input.Throttle));

	if (input.Throttle == 0) {
		//cvarManager->log("player is not throttling");
		return;
	}

	auto currentVelocity = myCar.GetVelocity();
	float currSpeed = myCar.GetForwardSpeed();

	if (!myCar.IsOnGround()) {
		return;
	}

	if (!enableSpeedMultiplier) {
		return;
	}

	if (std::abs(currSpeed) >= speedThreshold) {
		return;
	}

	if ((currSpeed <= 0 && input.Throttle < 0) || (currSpeed >= 0 && input.Throttle > 0)) {
		//cvarManager->log("Curr speed :" + std::to_string(currSpeed));
		currentVelocity *= speedMultiplier;
		myCar.SetVelocity(currentVelocity);
	}
	

}

