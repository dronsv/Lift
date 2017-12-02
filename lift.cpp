//
// Created by andrey on 02.12.17.
//

#include <iostream>
#include "lift.hpp"

#define NO_VALUE 99999

/*!
 * thread sleep for time in milliseconds
 * @param milliSeconds - time to sleep
 */
void sleepFor(unsigned int milliSeconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliSeconds));
}

#define BIND_FUNC(funcName) _processState = std::bind( &Lift:: funcName, this)

/*!
 * Constructor
 * @param firstLevel - minimal level number
 * @param lastLevel maximal level number
 * @param height - height of level
 * @param speed - speed of cabin
 * @param doorOpenTime - time of door open if no events received
 */
Lift::Lift(int firstLevel, int lastLevel, double height, double speed, double doorOpenTime) :
        _firstLevel(firstLevel),
        _lastLevel(lastLevel),
        _levelsCount(abs(_lastLevel - _firstLevel) + 1),
        _height(height),
        _speed(speed),
        _levelPassTimeMilliseconds(static_cast <decltype(_levelPassTimeMilliseconds)> (height / speed * 1000)),
        _doorOpensTimeMilliseconds(static_cast <decltype(_doorOpensTimeMilliseconds)> (doorOpenTime * 1000)),
        _state(abs(_lastLevel - _firstLevel) + 1, 0) {
    _thread = std::thread(&Lift::run, this);
}

/*!
 * destructor stop Lift thread
 */
Lift::~Lift() {
    _running = false;
    if (_thread.joinable()) {
        _newCommandCondition.notify_all();
        _thread.join();
    }
}

/*!
 * \brief process event from outside
 * @param level - event requested cabin level
 * @param fromCabin - flag that Even from cabin. Now this flag used to close door immediately
 */
void Lift::processEvent(int level, bool fromCabin) {

    if (!setLevelRequest(level, true)){
        return;
    }

    if (fromCabin) {
        BOOST_LOG_TRIVIAL(info) << "request from cabin to level: " << level;
        _cabinEventCondition.notify_all();
        _cabinControl = true;
    }
    else
        BOOST_LOG_TRIVIAL(info) << "request from outside to level: " << level;
    _newCommandCondition.notify_one();
}

int Lift::getDirection() {
    return _state._direction;
}

/*!
 * Get current level of lift cabin
 * @return level where floor of cabin now
 */
int Lift::getLevel() {
    return getIndexLevel(getCurrentIndex());
}

/*!
 * Get state of the Lift door
 * @return true if door open
 */
bool Lift::isDoorOpen() {
    return _state._doorsOpen;
}

void Lift::setDoorOpen(bool open) {
    _state._doorsOpen = open;
}

int Lift::getLevelIndex(int level) {
    return level - _firstLevel;
}

int Lift::getIndexLevel(int index) {
    return index + _firstLevel;
}

/*!
 * set request for given level
 * @param level -level ro set
 * @param state - state to set
 */
bool Lift::setLevelRequest(int level, bool state) {
    int index = getLevelIndex(level);
    if (index < 0 || index >= _levelsCount) {
        BOOST_LOG_TRIVIAL(error) << "level out of range: " << level;
        return false;
    }
    setIndexRequest(index, state);
    return true;
}

void Lift::setIndexRequest(int index, bool state) {
    _state._requests[index] = state;
}

bool Lift::getIndexRequest(int index) {
    return _state._requests[index];
}

void Lift::setCurrentIndex(int index) {
    _state._currentIndex = index;
}

int Lift::getCurrentIndex() {
    return _state._currentIndex;
}


void Lift::setDirection(int direction) {
    _state._direction = direction;
}

/*!
 * Checks is request to this index exists
 * @return true if request for current level true
 */
bool Lift::checkNeedDoorOpen() {
    return getIndexRequest(getCurrentIndex());
}

/*!
 * OpenDoor State function if got signal from cabin - closes door instantly
 */
void Lift::openDoor() {
    BOOST_LOG_TRIVIAL(info) << "openDoor at level: " << getLevel();
    setDoorOpen(true);

    std::unique_lock<std::mutex> locker(_cabinEventMutex);
    _cabinControl = false;
    _cabinEventCondition.wait_for(locker, std::chrono::milliseconds(_doorOpensTimeMilliseconds),
                                  [this]() -> bool { return _cabinControl; });

    setIndexRequest(getCurrentIndex(), false);
    setDoorOpen(false);
    BOOST_LOG_TRIVIAL(info) << "closeDoor at level: " << getLevel();


    switch (getDirection()) {
        case MOVE_UP:
            BIND_FUNC(moveUp);
            return;
        case MOVE_DOWN:
            BIND_FUNC(moveDown);
            return;
        default:
            BIND_FUNC(wait);
            return;
    }
}

/*!
 * Move Up state function
 */
void Lift::moveUp() {
    _state._direction = MOVE_UP;

    if (checkNeedDoorOpen()) {
        BIND_FUNC(openDoor);
        return;
    }


    if (_state.getClosest(MOVE_UP) != NO_VALUE) {
        BOOST_LOG_TRIVIAL(info) << "MoveUp at level: " << getLevel();
        sleepFor(_levelPassTimeMilliseconds);
        setCurrentIndex(getCurrentIndex() + 1);
        return;
    }

    if (_state.getClosest(MOVE_DOWN) != NO_VALUE) {
        BIND_FUNC(moveDown);
        return;
    }

    BIND_FUNC(wait);
}

/*
 * Move down state function
 */
void Lift::moveDown() {
    setDirection(MOVE_DOWN);

    if (checkNeedDoorOpen()) {
        BIND_FUNC(openDoor);
        return;
    }

    if (_state.getClosest(MOVE_DOWN) != NO_VALUE) {
        setCurrentIndex(getCurrentIndex() - 1);
        BOOST_LOG_TRIVIAL(info) << "MoveDown at level: " << getLevel();
        sleepFor(_levelPassTimeMilliseconds);
        return;
    }

    if (_state.getClosest(MOVE_UP) != NO_VALUE) {
        BIND_FUNC(moveUp);
        return;
    }

    BIND_FUNC(wait);
}

/*!
 * Waiting state function waiting for command until condition var notified or running stopped
 */
void Lift::wait() {
    setDirection(WAITING);
    while (_running) {
        std::unique_lock<std::mutex> locker(_newCommandMutex);

        auto closest = _state.getClosest(WAITING);
        if (closest != NO_VALUE) {
            if (closest > getCurrentIndex()) {
                BIND_FUNC(moveUp);
                return;
            } else {
                BIND_FUNC(moveDown);
                return;
            }
        }
        _newCommandCondition.wait_for(locker, std::chrono::seconds(1));
    }
}

/*!
 * Main Lift processing loop States change _processState variable by next state
 */
void Lift::run() {
    _running = true;
    BIND_FUNC(wait);
    while (_running) {
        _processState();
    }
}

/*!
 * State constructor
 * @param levelsCount - how much levels in building
 * @param levelIndex - start level index
 */
Lift::State::State(int levelsCount, int levelIndex) :
        _requests(static_cast<size_t>(levelsCount)),
        _currentIndex(levelIndex),
        _direction(WAITING),
        _doorsOpen(false) {
    _requests = false;
}
/*
 * Get nearest request index with given direction
 */
int Lift::State::getClosest(int direction) const {
    if (direction == MOVE_UP) {
        return getClosestUp();
    } else if (direction == MOVE_DOWN) {
        return getClosestDown();
    } else {
        auto down = getClosestDown();
        auto up = getClosestUp();
        if (abs(down - _currentIndex) < abs(up - _currentIndex))
            return down;
        else
            return up;
    }
}


/*
 * Get nearest request index with given direction down
 */
int Lift::State::getClosestDown() const {
    for (int indMin = _currentIndex; indMin >= 0; --indMin) {
        if (_requests[indMin])
            return indMin;
    }
    return NO_VALUE;
}

/*
 * Get nearest request index with given direction up
 */
int Lift::State::getClosestUp() const {
    size_t requestSize = _requests.size();
    for (int indMax = _currentIndex; indMax < requestSize; ++indMax) {
        if (_requests[indMax])
            return indMax;
    }
    return NO_VALUE;
}

#undef NO_VALUE
#undef BIND_FUNC