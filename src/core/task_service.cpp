#include "core/task_service.hpp"

namespace ortha {

TaskService::TaskService(TaskRepository& repo) : repo_(repo) {}

std::optional<Task> TaskService::complete(const std::string& id, Date today) {
    auto task = repo_.find(id);
    if (!task || task->done)
        return std::nullopt;

    std::optional<Task> next;
    if (task->recurrence && task->due) {
        // The series moves on to the next instance; the completed one stops recurring,
        // so completing it again (after a reopen) cannot create a second follow-up.
        next = *task;
        next->id.clear();
        next->completed_at.reset();
        for (auto& sub : next->subtasks) {
            sub.id.clear();
            sub.done = false;
        }
        const Date base = task->recurrence->anchor == Anchor::Due ? *task->due : today;
        next->due = next_due(*task->recurrence, base);
        task->recurrence.reset();
    }

    task->done = true;
    task->completed_at = now_utc_iso();
    repo_.database().transaction([&] {
        repo_.update(*task);
        if (next)
            repo_.insert(*next);
    });
    return next;
}

void TaskService::reopen(const std::string& id) {
    auto task = repo_.find(id);
    if (!task || !task->done)
        return;
    task->done = false;
    task->completed_at.reset();
    repo_.update(*task);
}

} // namespace ortha
