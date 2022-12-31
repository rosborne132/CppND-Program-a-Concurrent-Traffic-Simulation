#include <chrono>
#include <future>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>

#include "TrafficLight.h"


template <typename T>
T MessageQueue<T>::receive() {
    // FP.5a (Done): The method receive should use std::unique_lock<std::mutex> and _condition.wait()
    // to wait for and receive new messages and pull them from the queue using move semantics.
    // The received object should then be returned by the receive function.
    std::unique_lock<std::mutex> uLock(_mtx);

    _cond.wait(uLock, [this] { return !_queue.empty(); });

    T msg = std::move(_queue.back());
    _queue.pop_back();

    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg) {
    // FP.4a (Done): The method send should use the mechanisms std::lock_guard<std::mutex>
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    std::lock_guard<std::mutex> lock(_mtx);

    std::cout << "Msg: " << msg << " has been added to the queue." << std::endl;

    _queue.clear();

    _queue.emplace_back(std::move(msg));
    _cond.notify_one();
}

TrafficLight::TrafficLight() {
    _currentPhase = TrafficLightPhase::red;
    _messages = std::make_shared<MessageQueue<TrafficLightPhase>>();
}

void TrafficLight::waitForGreen() {
    // FP.5b (Done): add the implementation of the method waitForGreen, in which an infinite while-loop
    // runs and repeatedly calls the receive function on the message queue.
    // Once it receives TrafficLightPhase::green, the method returns.

    while (true) {
        TrafficLightPhase msg = _messages->receive();

        if (msg == TrafficLightPhase::green) return;
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase() { return _currentPhase; }

void TrafficLight::setCurrentPhase(TrafficLightPhase newPhase) { _currentPhase = newPhase; }

// Helper method that helps us generate a random time duration between 4000 and 6000 milliseconds.
// This method was created with the help of ChatGPT.
double TrafficLight::generateWaitDuration() {
    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<double> dist(4000, 6000);

    return dist(rng);
}

void TrafficLight::togglePhase() {
    TrafficLightPhase currentPhase = getCurrentPhase();
    TrafficLightPhase newPhase = (currentPhase == TrafficLightPhase::green)
        ? TrafficLightPhase::red
        : TrafficLightPhase::green;

    setCurrentPhase(newPhase);
}

void TrafficLight::simulate() {
    // FP.2b (Done): Finally, the private method „cycleThroughPhases“ should be started in a thread when the
    // public method „simulate“ is called. To do this, use the thread queue in the base class.
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases() {
    // FP.2a (Done): Implement the function with an infinite loop that measures the time between two loop cycles
    // and toggles the current phase of the traffic light between red and green.
    std::unique_lock<std::mutex> uLock(_mtx);
    std::future<void> future;

    // init values
    std::chrono::_V2::system_clock::time_point currentTime = std::chrono::system_clock::now();
    double waitDuration = TrafficLight::generateWaitDuration();

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::chrono::_V2::system_clock::time_point newCurrentTime = std::chrono::system_clock::now();
        double currentTimeDiff = (newCurrentTime - currentTime).count();

        if (currentTimeDiff >= waitDuration) {
            TrafficLight::togglePhase();
            TrafficLightPhase currentPhase = getCurrentPhase();

            // FP.4b (Done): Send an update method to the message queue using move semantics.
            // The cycle duration should be a random value between 4 and 6 seconds.
            // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.
            future = std::async(&MessageQueue<TrafficLightPhase>::send, _messages, std::move(currentPhase));
            future.wait();

            // Reset values for next cycle.
            waitDuration = TrafficLight::generateWaitDuration();
            currentTime = std::chrono::system_clock::now();
        }
    }
}

