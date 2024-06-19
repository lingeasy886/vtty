#include "vtty_task.h"
#include "vtty.h"

#include "pthread.h"

#include <thread>
#include <future>
#include <algorithm>
#include <functional>

static void *__pthread_template(void *arg)
{
    auto f = (std::function<void()> *)arg;
    f->operator()();
    delete f;
    return nullptr;
}

const uint32_t VttyTask::new_task(cmd_desc_t &cmd_desc, const std::string args_raw, const std::vector<std::string> &args)
{
    task_id_t taskidx;
    {
        std::unique_lock<std::mutex> lock(__task_mtx);
        if (__task_list.size() > __max_task_count && !(cmd_desc.name == "kill" || cmd_desc.name == "ps"))
        {
            return 0; // 任务队列已满
        }
        taskidx = ++__task_count;
        if (taskidx == 0)
        {
            taskidx = ++__task_count; // 处理id为0的情况
        }
        __task_list.emplace_back(taskidx, cmd_desc, args_raw, args, true);
    }

    __task_cv.notify_one();
    return taskidx;
}

const std::vector<VttyTask::task_id_t> VttyTask::get_task_id_list(const std::string &cmd_name)
{
    std::vector<task_id_t> result_vec;
    std::unique_lock<std::mutex> lock(__task_mtx);

    for (auto it = __task_list.begin(); it != __task_list.end(); ++it)
    {
        if (it->cmd_desc.name == cmd_name && it->cmd)
        {
            result_vec.push_back(it->cmd->task_id);
        }
    }
    return result_vec;
}

const bool VttyTask::del_task(const task_id_t task_id)
{
    sig_task(std::vector<task_id_t>{task_id}, sig_type_t::exit, "");
    return true;
}

const std::vector<uint32_t> VttyTask::del_task(const std::string &cmd_name)
{
    std::vector<task_id_t> wait_kill_vec{get_task_id_list(cmd_name)};
    sig_task(wait_kill_vec, sig_type_t::exit, "");
    return wait_kill_vec;
}

const void VttyTask::sig_task(const std::vector<task_id_t> &task_id_list, const sig_type_t msg_type, const std::string &msg)
{
    std::unique_lock<std::mutex> lock(__task_mtx);
    for (auto task_id : task_id_list)
    {
        auto it = std::find_if(__task_list.begin(), __task_list.end(), [&](const task_info_t &task)
                               { return task.cmd->task_id == task_id; });
        if (it != __task_list.end())
        {
            it->sig_queue.emplace(msg_type, msg); // 将消息写入到对应任务的对列中
        }
    }
    __task_cv.notify_one();
}

const void VttyTask::sig_task(const VttyTask::task_id_t task_id, const sig_type_t msg_type, const std::string &msg)
{
    sig_task(std::vector<task_id_t>{task_id}, msg_type, msg);
}

const std::vector<std::tuple<VttyTask::task_id_t, std::string, std::string, std::chrono::time_point<std::chrono::system_clock>, VttyTask::task_state_t>> VttyTask::get_task_info()
{
    std::vector<std::tuple<task_id_t, std::string, std::string, std::chrono::time_point<std::chrono::system_clock>, task_state_t>> result;

    std::unique_lock<std::mutex> lock(__task_mtx);
    for (auto it = __task_list.begin(); it != __task_list.end(); ++it)
    {
        result.emplace_back(it->cmd->task_id, it->cmd_desc.name, it->args_raw, it->cmd->create_time, it->state);
    }
    return result;
}

bool VttyTask::task_exists(const std::string &cmd_name)
{
    std::unique_lock<std::mutex> lock(__task_mtx);
    return std::find_if(__task_list.begin(), __task_list.end(), [&cmd_name](const task_info_t &task_info) -> bool
                        { return task_info.cmd_desc.name == cmd_name; }) != __task_list.end();
}

bool VttyTask::task_exists(const VttyTask::task_id_t task_id)
{
    std::unique_lock<std::mutex> lock(__task_mtx);
    return std::find_if(__task_list.begin(), __task_list.end(), [&task_id](const task_info_t &task_info) -> bool
                        { return task_info.cmd->task_id == task_id; }) != __task_list.end();
}

const std::string VttyTask::state_to_str(task_state_t state)
{
    switch (state)
    {
    case task_state_t::RUNNING:
        return "RUNNING";
    case task_state_t::WAITING:
        return "WAITING";
    case task_state_t::COMPLETED:
        return "COMPLETED";
    case task_state_t::DELETED:
        return "DELETED";
    case task_state_t::DEATHED:
        return "DEATHED";
    default:
        return "UNKNOWN";
    }
}

void VttyTask::__handler()
{
    while (__running)
    {
        // 等待__task_cv的信号再执行
        std::unique_lock<std::mutex> lock(__task_mtx);
        __task_cv.wait(lock);
        auto it = __task_list.begin();
        while (it != __task_list.end())
        {
            switch (it->state)
            {
            case task_state_t::WAITING:
                // 启动任务
                {
                    pthread_t ntid;
                    auto fun = std::bind(&VttyTask::__task_template, this, std::ref(*it));
                    pthread_attr_t attr;
                    pthread_attr_init(&attr);
                    pthread_attr_setstacksize(&attr, 1024 * 10); // 设置栈大小
                    pthread_create(&ntid, &attr, __pthread_template, new std::function<void()>(fun));
                    pthread_detach(ntid);
                    pthread_attr_destroy(&attr);
                }
                break;
            case task_state_t::RUNNING:
            {
                // 处理消息
                while (!it->sig_queue.empty())
                {
                    sig_type_t msg_type{sig_type_t::msg};
                    std::string msg_data{""};
                    std::tie(msg_type, msg_data) = it->sig_queue.front(); // 弹出队列中的消息
                    it->sig_queue.pop();
                    it->cmd->on_sig(msg_type, msg_data); // 处理信号
                }
            }
            break;
            case task_state_t::COMPLETED:
            {
                // 处理结束
                it->state = task_state_t::DELETED;
            }
            break;
            case task_state_t::DELETED:
            {
                // 处理任务删除操作
                it = __task_list.erase(it); // 删除当前项，并返回下一项的迭代器
                continue;
            }
            break;
            default:
                break;
            }
            ++it; // 移动到下一项
        }
    }
}

void VttyTask::__task_template(VttyTask::task_info_t &task)
{
    if (task.cmd == nullptr)
    {
        __vtty->println("Wrong task address!\n");
        return;
    }
    task.state = task_state_t::RUNNING;
    task.cmd->init();
    auto result = task.cmd->execute();
    task.cmd->finish_time = std::chrono::system_clock::now();
    task.state = task_state_t::COMPLETED;
    if (!result.empty())
    {
        __vtty->println(result);
    }
    __task_cv.notify_one(); // 通知等待的线程
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

VttyTask::VttyTask(Vtty *vtty, const int max_task_count) : __vtty(vtty), __max_task_count(max_task_count)
{
    __running = true;
    std::thread(&VttyTask::__handler, this).detach(); // 启动任务系统
}

VttyTask::~VttyTask()
{
    __running = false;
}

// 初始化task_count的值
#if __cplusplus < 201703L
uint32_t VttyTask::__task_count = 0; // 任务ID，所有VTTY共享一个task计数
#endif
