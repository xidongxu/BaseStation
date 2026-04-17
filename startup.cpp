#include "startup.h"
#include "server.h"
#include "configure.h"
#include <lyra/lyra.hpp>
#include <cli/cli.h>
#include <cli/clilocalsession.h>
#include <cli/standaloneasioscheduler.h>
namespace cli {
using MainScheduler = StandaloneAsioScheduler;
}
using namespace std;
using namespace cli;

#define LOG_TAG "startup"
#include "logger.h"

static void prepare() {
#ifdef _WIN32
#include <cstdlib>
#include <crtdbg.h>
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
#endif
}

static std::unique_ptr<cli::Menu> menu() {
    auto rootMenu = make_unique< Menu >("cli");
    rootMenu->Insert(
        "version",
        [](std::ostream& out) { out << BASE_STATION_VERSION << "\n"; },
        "Print Version");
    rootMenu->Insert(
        "answer",
        [](std::ostream& out, int x) { out << "The answer is: " << x << "\n"; },
        "Print the answer to Life, the Universe and Everything");
    rootMenu->Insert(
        "stop",
        [](std::ostream& out) { Server::instance().close(); },
        "Server stop");
    return rootMenu;
}

int main(int argc, char* argv[]) {
    bool help = false;
    bool version = false;
    uint16_t port = 5566;
    bool specified_port = false;
    std::string configure = "configure.json";

    prepare();
    auto arg = lyra::cli()
        | lyra::help(help)["-h"]["--help"]("Show help information")
        | lyra::opt([&](uint16_t p) { port = p, specified_port = true; }, "port")["-p"]["--port"]("Set server port number")
        | lyra::opt(configure, "path")["-c"]["--config"]("The path to the configure.json")
        | lyra::opt(version)["-v"]["--version"]("Show version.");
    auto res = arg.parse({ argc, argv });
    if (!res) {
        LogError() << "arg parse failed:" << res.message();
        return -1;
    }
    if (help) {
        LogInfo() << arg;
        return 0;
    }
    if (version) {
        LogInfo() << BASE_STATION_VERSION;
        return 0;
    }
    if (configure.empty()) {
        LogError() << "configure.json path empty:" << configure;
        LogError() << arg;
        return -1;
    }
    
    auto& Config = Configure::instance();
    if (!Config.load(configure)) {
        LogError() << "configure.json parse failed";
        return -2;
    }
    if (!specified_port) {
        port = Config.port();
    }

    auto &Server = Server::instance();
    Server.start(port);

    MainScheduler scheduler;
    Cli cli(std::move(menu()));
    cli.EnterAction([](auto& out) { out << "===> Base Station Simulator Cli <===\n"; });
    cli.ExitAction([](auto& out) { out << "\nBase Station Simulator is quit now.\n"; });
    CliLocalSession session(cli, scheduler, std::cout);
    session.ExitAction([&scheduler](auto& out) { scheduler.Stop(); });
    scheduler.Run();

    Server.close();
    return 0;
}
