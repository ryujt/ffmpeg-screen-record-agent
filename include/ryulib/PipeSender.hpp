#pragma once

#include <Windows.h>
#include <string>
#include <ryulib/base.hpp>
#include <ryulib/Worker.hpp>

using namespace std;
using namespace ryulib;

class PipeSender {
private:
    const static int TASK_OPEN = 1;
    const static int TASK_CLOSE = 2;
    const static int TASK_SEND = 3;

public:
    PipeSender()
    {
        worker_.setOnTask([&](int task, const string text, const void* data, int size, int tag) {
            switch (task) {
                case TASK_OPEN: do_open((HANDLE) tag);  break;
                case TASK_CLOSE: do_close(); break;
                case TASK_SEND: do_send((Memory*) data); break;
            }
        });
    }

    ~PipeSender()
    {
        worker_.terminateAndWait();
    }

    bool open(LPCWSTR name)
    {
        HANDLE pipe = CreateNamedPipe(name,
            PIPE_ACCESS_OUTBOUND,
            PIPE_TYPE_BYTE,
            1, 0, 0, 0, NULL);
        if (pipe == INVALID_HANDLE_VALUE) {
            return false;
        }

        worker_.add(TASK_OPEN, nullptr, 0, (int) pipe);

        return true;
    }

    void close()
    {
        worker_.add(TASK_CLOSE);
    }

    void send(const void* data, int size)
    {
        if (is_connected_) worker_.add(TASK_SEND, new Memory(data, size), 0, 0);
    }

    bool isConnected() { return is_connected_; }

private:
    HANDLE pipe_ = INVALID_HANDLE_VALUE;
    Worker worker_;
    bool is_connected_ = false;

    void do_open(HANDLE pipe)
    {
        pipe_ = pipe;
        if (ConnectNamedPipe(pipe, NULL) == FALSE) {
            CloseHandle(pipe);
            pipe_ = INVALID_HANDLE_VALUE;
            return;
        }

        is_connected_ = true;
    }

    void do_close()
    {
        is_connected_ = false;

        if (pipe_ == INVALID_HANDLE_VALUE) return;

        FlushFileBuffers(pipe_);
        CloseHandle(pipe_);
        pipe_ = INVALID_HANDLE_VALUE;
    }

    void do_send(Memory* data)
    {
        if (pipe_ == INVALID_HANDLE_VALUE) return;

        DWORD dwWritten;
        WriteFile(pipe_, data->getData(), data->getSize(), &dwWritten, NULL);
        delete data;
    }
};
