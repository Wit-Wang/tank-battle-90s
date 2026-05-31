#include "GameServer.h"
#include "server/ServerConfig.h"
#include <cstdio>
#include <cstdlib>
#include <csignal>

static volatile bool g_running = true;

void SignalHandler(int) {
    g_running = false;
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, SignalHandler);
#ifdef SIGPIPE
    std::signal(SIGPIPE, SIG_IGN);  // Ignore broken pipe on Linux
#endif

    uint16_t port = SERVER_PORT;
    if (argc > 1) {
        port = static_cast<uint16_t>(std::atoi(argv[1]));
    }

    printf("===========================================\n");
    printf("  Tank Battle 90s - Server\n");
    printf("===========================================\n");
    printf("Port: %d\n", port);
    printf("\n");

    GameServer server;
    if (!server.Start(port)) {
        printf("Failed to start server on port %d\n", port);
        return 1;
    }

    printf("Server running. Press 'q' or Ctrl+C to stop.\n");
    printf("\n");

    server.SetRunningFlag(&g_running);
    server.Run();

    server.Stop();
    printf("Server stopped.\n");
    return 0;
}
