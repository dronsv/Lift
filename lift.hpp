//
// Created by andrey on 02.12.17.
//

#ifndef LIFT_LIFT_HPP
#define LIFT_LIFT_HPP

#include <condition_variable>
#include <mutex>
#include <atomic>
#include <thread>
#include <valarray>

#define MOVE_UP (1)
#define WAITING (0)
#define MOVE_DOWN ((-1))

#define  BOOST_LOG_TRIVIAL(info) std::cerr << "\n" << #info <<"\t"

#define BIND_FUNC(funcName) _processState = std::bind( &Lift:: funcName, this)

/*!
 * class implemented Lift emulator. It's use state based pattern
 * Use valarray for optimization reasons exclude sync on eventProcessing
 * and decrease number of memory managment operations
 */
class Lift {
public:
    explicit Lift(int firstLevel, int lastLevel, double height, double speed, double doorOpenTime);

    ~Lift();;

    void processEvent(int level, bool fromCabin);

    int getDirection();

    int getLevel();

    bool isDoorOpen();


private:

    void operator=(const Lift &) = delete;

    Lift(const Lift &) = delete;

    void setDoorOpen(bool open);

    int getLevelIndex(int level);

    int getIndexLevel(int index);

    bool setLevelRequest(int level, bool state);

    void setIndexRequest(int index, bool state);

    bool getIndexRequest(int index);

    void setCurrentIndex(int index);

    int getCurrentIndex();

    void setDirection(int direction);


    bool checkNeedDoorOpen();

    void openDoor();

    void moveUp();

    void moveDown();

    void wait();

    std::function<void()> _processState;///< current state

    void run();

    int _firstLevel;///< number of base level
    int _lastLevel;///< number of top level
    int _levelsCount;///< total levels count
    double _height;///< height of level
    double _speed;///< speed of lift
    unsigned int _levelPassTimeMilliseconds;///< time to pass one level state
    unsigned int _doorOpensTimeMilliseconds;///< time of open door state

    struct State {
        explicit State(int levelsCount, int levelIndex);

        int getClosest(int direction) const;

        int getClosestDown() const;

        int getClosestUp() const;

        std::valarray<bool> _requests;
        int _currentIndex = 0;
        int _direction = WAITING;
        bool _doorsOpen = false;
    };

    State _state;///\variable representing current lift state


    std::mutex _newCommandMutex;///\brief Mutex for new command condition variable
    std::condition_variable _newCommandCondition;///\brief used for wake up waiting state

    std::mutex _cabinEventMutex;///< mutex for _cabinEventCondition
    std::condition_variable _cabinEventCondition;///< used for close door instantly after cabin event
    bool _cabinControl = false;///< cabinEventCondition control flag

    std::thread _thread;///< main lift thread
    bool _running = false;
#undef BIND_FUNC
};

#endif //LIFT_LIFT_HPP
