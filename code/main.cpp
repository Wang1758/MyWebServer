#include <unistd.h>
#include "server/webserver.h"

int main() {
    WebServer server(6699, 3, 60000, false,
                     3306, "root", "root", "yourdb",
                     15, 6, true, 1, 1000);
    server.Start();
}