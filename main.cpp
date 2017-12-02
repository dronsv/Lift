#include <iostream>
#include <valarray>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <csignal>
#include "lift.hpp"

void printUsage() {
    BOOST_LOG_TRIVIAL( info ) << "this program needs 4 parameters:\n"
                             << "topLevel: integer\n"
                             << "levelHeight (m): double\n"
                             << "Lift speed(m/s): double\n"
                             << "door open time(s): double\n";

    BOOST_LOG_TRIVIAL( info ) << "A-Z: buttons in lift (A - first level)\n";
    BOOST_LOG_TRIVIAL( info ) << "a-z: buttons outside lift (A - first level)\n";
    BOOST_LOG_TRIVIAL( info ) << "= : exit from program\n";
}


void handler(int signal) {
    BOOST_LOG_TRIVIAL( info ) << "signal received: " << signal;
    exit(-2);
};

int main(int argc, char *argv[]) {
    BOOST_LOG_TRIVIAL(info) << "program started";

    if (argc < 5) {
        BOOST_LOG_TRIVIAL(error) << "error in parse parameters: ";
        return -1;
    }

    int maxLevel(10);
    double height(3), speed(2), openTime(5);


    try {
        maxLevel = std::stoi(argv[1]);
        height = std::stod(argv[2]);
        speed = std::stod(argv[3]);
        openTime = std::stod(argv[4]);
    }
    catch (std::exception &exception) {
        BOOST_LOG_TRIVIAL(error) << "error in parse parameters: " << exception.what();
        printUsage();
        exit(-1);
    }

    Lift lift(1, maxLevel, height, speed, openTime);
    std::string input;

    //Предусматриваем выход по сигналам, но с учетом использования cin - пофик
    std::signal(SIGINT, handler);
    std::signal(SIGTERM, handler);


    std::atomic_bool running(true);
    do {
        std::cin >> input;
        char ch = input[0];
        if (input[0] >= 'a' && input[0] <= 'z') {
            lift.processEvent(ch - 'a' + 1, false);
        }
        if (input[0] >= 'A' && input[0] <= 'Z') {
            lift.processEvent(ch - 'A' + 1, true);
        }

    } while ((input != "=") && (running == true));

    return 0;
}