#include "ServerMainloop.hpp"

#include "Engine.hpp"
#include "logic/scripting/scripting.hpp"
#include "logic/LevelController.hpp"
#include "interfaces/Process.hpp"
#include "debug/Logger.hpp"
#include "world/Level.hpp"
#include "world/World.hpp"
#include "util/platform.hpp"

#include <chrono>

using namespace std::chrono;

static debug::Logger logger("mainloop");

inline constexpr int TPS = 20;

ServerMainloop::ServerMainloop(Engine& engine) : engine(engine) {
}

ServerMainloop::~ServerMainloop() = default;

void ServerMainloop::run() {
    const auto& coreParams = engine.getCoreParameters();
    auto& time = engine.getTime();

    if (coreParams.scriptFile.empty()) {
        logger.info() << "nothing to do";
        return;
    }
    engine.setLevelConsumer([this](auto level, auto) {
        setLevel(std::move(level));
    });

    logger.info() << "starting test " << coreParams.scriptFile.string();
    auto process = scripting::start_coroutine(
        "script:" + coreParams.scriptFile.filename().u8string()
    );

    double targetDelta = 1.0 / static_cast<double>(TPS);
    double delta = targetDelta;
    auto begin = system_clock::now();
    auto startupTime = begin;

    while (!engine.isQuitSignal()) {
        if (process) {
            try {
                if (process->isActive()) {
                    process->update();
                } else {
                    process.reset();  // скрипт закончился — не трогаем больше
                }
            } catch (const std::exception& e) {
                logger.error() << "coroutine crashed: " << e.what();
                if (process) process->terminate();
                process.reset();
            }
        }
        if (coreParams.testMode) {
            time.step(delta);
        } else {
            auto now = system_clock::now();
            time.update(
                duration_cast<microseconds>(now - startupTime).count() / 1e6);
            delta = time.getDelta();
        }
        process->update();
        if (controller) {
            controller->getLevel()->getWorld()->updateTimers(delta);
            controller->update(glm::min(delta, 0.2), false);
        }
        engine.postUpdate();

        if (!coreParams.testMode) {
            auto end = system_clock::now();
            int64_t millis = targetDelta * 1000 - 
                duration_cast<microseconds>(end - begin).count() / 1000;
            if (millis > 0) {
                platform::sleep(millis);
            }
            begin = system_clock::now();
        }
    }
    logger.info() << "script finished";
}

void ServerMainloop::setLevel(std::unique_ptr<Level> level) {
    if (level == nullptr) {
        controller->onWorldQuit();
        engine.getPaths().setCurrentWorldFolder("");
        controller = nullptr;
    } else {
        controller = std::make_unique<LevelController>(
            &engine, std::move(level), nullptr
        );
    }
}
