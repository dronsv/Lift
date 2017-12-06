#include <iostream>
#include <csignal>
#include "lift.hpp"

void printUsage() {
    BOOST_LOG_TRIVIAL( info ) << "This program needs 4 parameters:\n"
                              << "topLevel: integer\n"
                              << "levelHeight (m): double\n"
                              << "Lift speed(m/s): double\n"
                              << "door open time(s): double\n"
                              << "example Lift 10 3.0 2.0 7.0\n"
                              << "decimal delimiter depends on system settings\n";

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
        printUsage();
        return -1;
    }

    int maxLevel(10);
    double height(3), speed(2), openTime(5);


    try {
        maxLevel = std::stoi(argv[1]);
        if (maxLevel <5 || maxLevel >20)
            throw "level range [1..5]";
        height = std::stod(argv[2]);
        if ( height <= 0)
            throw "height must be >=0";
        speed = std::stod(argv[3]);
        if ( speed <= 0)
            throw "speed must be >=0";
        openTime = std::stod(argv[4]);
        if ( openTime <= 0)
            throw "openTime must be >=0";
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