#include <iostream>
#include <unistd.h>
#include "PollScheduler.h"
#include "connection/ConnectionScheduler.h"
#include "client/ClientScheduler.h"
#include <spdlog/spdlog.h>
#include <signal.h>
#include "Configuration.h"

bool active = true;
ConnectionScheduler connectionScheduler("127.0.0.1", 8084);
ClientScheduler clientScheduler;
CashScheduler cashScheduler;
PollScheduler pollScheduler(connectionScheduler, clientScheduler, cashScheduler);

void eventCashHandler(int signo, siginfo_t *info, void *context) {
    if (signo == SIGALRM) {
        write(cashScheduler.cash_event_fd[1], "hello", 5);
        alarm(PERIOD_CASH_EVENT);
    }
    if (signo == SIGTERM) {
        spdlog::critical("STOP PROXY {}",errno);
        active = false;
        pollScheduler.destroy();
        pollScheduler.builder();
        cashScheduler.destroy();
        connectionScheduler.disconnect();
        exit(EXIT_SUCCESS);
    }
}

int main() {
    switch (pollScheduler.open_connect()) {
        case OPEN_CONNECTION_SUCCESS: {
            spdlog::info("PROXY START");
            alarm(PERIOD_CASH_EVENT);

            struct sigaction act = {0};

            act.sa_flags = SIGINT;
            act.sa_sigaction = &(eventCashHandler);
            if (sigaction(SIGALRM, &act, NULL) == -1) {
                perror("sigaction");
                exit(EXIT_FAILURE);
            }
            if (sigaction(SIGTERM, &act, NULL) == -1) {
                perror("sigaction");
                exit(EXIT_FAILURE);
            }
            while (active) {
                pollScheduler.builder();
                pollScheduler.executor();
            }
        }
        case OPEN_CONNECTION_FAILED: {
            spdlog::critical("PROXY FAILED START");
        }
    }
}
