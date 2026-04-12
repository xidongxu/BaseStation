#include "startup.h"
#include <lyra/lyra.hpp>
#include <cli/cli.h>
#include <cli/clilocalsession.h>
#include <cli/standaloneasioscheduler.h>
namespace cli {
using MainScheduler = StandaloneAsioScheduler;
}
using namespace std;
using namespace cli;

std::unique_ptr<cli::Menu> menu() {
    auto rootMenu = make_unique< Menu >("cli");
    rootMenu->Insert(
        "hello",
        [](std::ostream& out) { out << "Hello, world\n"; },
        "Print hello world");
    rootMenu->Insert(
        "answer",
        [](std::ostream& out, int x) { out << "The answer is: " << x << "\n"; },
        "Print the answer to Life, the Universe and Everything ");

    return rootMenu;
}

int main(int argc, char* argv[]) {
    bool help = false;
    uint16_t port = 5566;

    auto arg = lyra::cli()
        | lyra::help(help)["-h"]["--help"]("Show help information")
        | lyra::opt(port, "port")["-p"]["--port"]("Set server port number");
    auto res = arg.parse({ argc, argv });
    if (!res) {
        LogInfo() << "Arg parse failed: " << res.message();
        return -1;
    }
    if (help) {
        LogInfo() << arg;
        return 0;
    }
    auto &server = server::instance();
    server.start(port);

    MainScheduler scheduler;
    Cli cli(std::move(menu()));
    cli.EnterAction([](auto& out) { out << "===> Base Station Simulator Cli <===\n"; });
    cli.ExitAction([](auto& out) { out << "\nBase Station Simulator is quit now.\n"; });
    CliLocalSession session(cli, scheduler, std::cout);
    session.ExitAction([&scheduler](auto& out) { scheduler.Stop(); });
    scheduler.Run();

    server.close();
    return 0;
}
