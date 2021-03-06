/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2017 Mattia Basaglia
 * \license
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef MELANOBOT_MODULES_TIMER_CONNECTION
#define MELANOBOT_MODULES_TIMER_CONNECTION

#include "network/async_service.hpp"
#include "network/time.hpp"
#include "concurrency/concurrency.hpp"

namespace timer {

/**
 * \brief Item for TimerQueue
 */
class TimerItem
{
public:
    network::Time timeout;        ///< Time at which the item will be executed
    std::function<void()> action; ///< Action to be executed
    const void* owner = nullptr;  ///< Pointer to an object that owns the action

    TimerItem(network::Time timeout,
              std::function<void()> action,
              const void* owner = nullptr)
        : timeout(timeout), action(std::move(action)), owner(owner)
    {
    }

    /**
     * \brief Comparator used to create the heap
     */
    bool operator<(const TimerItem& rhs) const
    {
        return timeout > rhs.timeout;
    }
};

/**
 * \brief Asynchronous queue that executes functions at given times
 */
class TimerQueue : public AsyncService, public melanolib::Singleton<TimerQueue>
{
public:
    /**
     * \brief Adds an item to the queue
     */
    void push(TimerItem&& item);

    template<class... Args>
        void push(Args&&... args)
        {
            push(TimerItem(std::forward<Args>(args)...));
        }

    /**
     * \brief Removes all items owned by the given object
     */
    void remove(const void* owner);


    /**
     * \brief Stops the thread
     */
    void stop() override;

    /**
     * \brief Starts the thread
     */
    void start() override;

    bool running() const override;

    std::string name() const override;

    void initialize(const Settings& settings) override {}

private:
    friend ParentSingleton;

    TimerQueue()
    {
    }

    ~TimerQueue()
    {
        stop();
    }

    /**
     * \brief Thread function
     */
    void run();

    /**
     * \brief Tries to execute the top item
     * \param lock std::unique_lock<std::mutex> for events_mutex, will be released if an action is executed
     * \return \b true if the top item has been executed
     */
    bool tick(std::unique_lock<std::mutex>& lock);

    std::vector<TimerItem> items;       ///< Items (heap)
    std::condition_variable condition;  ///< Activated on timeout of the next items or when the item heap changes
    std::mutex events_mutex;            ///< Protects \p items, locked by \p condition
    std::thread thread;                 ///< Thread for run()

    /**
     * \brief Type of action to perform when \p condition awakens
     */
    enum class TimerAction
    {
        Noop,   ///< No action, release execution. For when items might have changed
        Tick,   ///< Execute the top item
        Die     ///< Terminate the thread
    };

    std::atomic<TimerAction> timer_action;

    struct EditLock;
};

} // namespace timer
#endif // MELANOBOT_MODULES_TIMER_CONNECTION
