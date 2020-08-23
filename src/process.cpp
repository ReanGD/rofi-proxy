#include "process.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <glib.h>
#pragma GCC diagnostic pop

#include <fcntl.h>
#include <unistd.h>

#include "defer.h"
#include "logger.h"
#include "exception.h"


namespace {

static void onProcessExit(GPid pid, gint status, gpointer context) {
    if (context != nullptr) {
        auto* handler = reinterpret_cast<ProcessHandler*>(context);
        handler->OnProcessExit(pid, (g_spawn_check_exit_status(status, nullptr) == TRUE));
    }
}

static int onProcessInput(GIOChannel *source, GIOCondition /* condition */, gpointer context) {
    static GString* buffer = g_string_sized_new(1024);

    GError* error = nullptr;
    Defer _([&](...) mutable {
        if (error != nullptr) {
            g_error_free(error);
        }
    });

    gunichar unichar;
    GIOStatus status = g_io_channel_read_unichar(source, &unichar, &error);

    while (status == G_IO_STATUS_NORMAL) {
        g_string_append_unichar(buffer, unichar);
        if (unichar == '\n') {
            if (buffer->len > 1) { //input is not an empty line
                if (context != nullptr) {
                    auto* handler = reinterpret_cast<ProcessHandler*>(context);
                    buffer->str[buffer->len - 1] = '\0';
                    handler->OnReadLine(buffer->str);
                }
            }
            g_string_set_size(buffer, 0);
        }
        status = g_io_channel_read_unichar(source, &unichar, &error);
    }

    // G_IO_STATUS_AGAIN == nothing to read
    if ((status != G_IO_STATUS_AGAIN) && (context != nullptr)) {
        auto* handler = reinterpret_cast<ProcessHandler*>(context);
        if (error != nullptr) {
            handler->OnReadLineError(error->message);
        } else {
            handler->OnReadLineError("unknown error");
        }
    }

    return G_SOURCE_CONTINUE;
}

}

Process::Process(ProcessHandler* handler, const std::shared_ptr<Logger>& logger)
    : m_readFd(STDIN_FILENO)
    , m_writeFd(STDOUT_FILENO)
    , m_handler(handler)
    , m_logger(logger) {

}

Process::~Process() {
    if (m_readChWatcher != 0) {
        g_source_remove(m_readChWatcher);
        m_readChWatcher = 0;
    }

    if (m_readCh != nullptr) {
        g_free(m_readCh);
        m_readCh = nullptr;
    }

    if (m_writeCh != nullptr) {
        g_free(m_writeCh);
        m_writeCh = nullptr;
    }

    if (m_readFd != STDIN_FILENO) {
        close(m_readFd);
        m_readFd = STDIN_FILENO;
    }

    if (m_writeFd != STDOUT_FILENO) {
        close(m_writeFd);
        m_writeFd = STDOUT_FILENO;
    }

    if (m_pid >= 0) {
        g_spawn_close_pid(m_pid);
        m_pid = -1;
    }

    m_handler = nullptr;
    m_logger.reset();
}

void Process::Start(const char *command) {
    try {
        StartImpl(command);
    } catch(const std::exception& e) {
        m_logger->Error("Unable to start child process \"%s\"", command);
        throw ProxyError("Unable to start child process \"%s\"", command);
    }
}

void Process::Write(const char* text) {
    GError *error = nullptr;
    Defer _([&](...) mutable {
        if (error != nullptr) {
            g_error_free(error);
        }
    });

    gsize bytesWitten;
    if (g_io_channel_write_chars(m_writeCh, text, -1, &bytesWitten, &error) != G_IO_STATUS_NORMAL) {
        if (error != nullptr) {
            throw ProxyError(error->message);
        } else {
            throw ProxyError("unknown error");
        }
    }

    if (g_io_channel_write_unichar(m_writeCh, '\n', &error) != G_IO_STATUS_NORMAL) {
        if (error != nullptr) {
            throw ProxyError(error->message);
        } else {
            throw ProxyError("unknown error");
        }
    }

    if (g_io_channel_flush(m_writeCh, &error) != G_IO_STATUS_NORMAL) {
        if (error != nullptr) {
            throw ProxyError(error->message);
        } else {
            throw ProxyError("unknown error");
        }
    }
}

void Process::Kill() {
    m_logger->Debug("Send SIGTERM to child process %" G_PID_FORMAT, m_pid);
    if (m_pid >= 0) {
        kill(m_pid, SIGTERM);
    } else if (m_handler != nullptr) {
        m_handler->OnProcessExit(0, true);
    }
}

void Process::StartImpl(const char* command) {
    if (command != nullptr) {
        char **argv = nullptr;
        GError *error = nullptr;

        Defer _([&](...) mutable {
            if (argv != nullptr) {
                g_strfreev(argv);
            }
            if (error != nullptr) {
                g_error_free(error);
            }
        });

        if (g_shell_parse_argv(command, nullptr, &argv, &error) == FALSE) {
            throw ProxyError("unable to parse arg '-proxy-cmd': %s", error->message);
        }
        Spawn(argv);
        m_logger->Debug("Start child process \"%s\"", command);
    } else {
        m_logger->Debug("Use stdin and stdout instead child process");
    }

    try {
        SetNonBlockFlag(m_readFd);
        m_readCh = g_io_channel_unix_new(m_readFd);
        m_writeCh = g_io_channel_unix_new(m_writeFd);
        m_readChWatcher = g_io_add_watch(m_readCh, G_IO_IN, onProcessInput, m_handler);
    } catch(const std::exception& e) {
        if (m_pid >= 0) {
            kill(m_pid, SIGTERM);
        }
        throw;
    }

    if (m_pid >= 0) {
        g_child_watch_add(m_pid, onProcessExit, m_handler);
    }
}

void Process::Spawn(char **argv) {
    char **envp = nullptr;
    void* userData = nullptr;
    int* errorFDPtr = nullptr;
    const char* workingDirectory = nullptr;
    GSpawnChildSetupFunc childSetup = nullptr;
    GSpawnFlags flags = static_cast<GSpawnFlags>(G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH);

    GError *error = nullptr;
    Defer _([&](...) mutable {
        if (error != nullptr) {
            g_error_free(error);
        }
    });

    if (g_spawn_async_with_pipes(workingDirectory, argv, envp, flags, childSetup, userData,
                                &m_pid, &m_writeFd, &m_readFd, errorFDPtr, &error) == FALSE) {
        throw ProxyError(error->message);
    }
}

void Process::SetNonBlockFlag(int fd) {
    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) != 0) {
        throw ProxyError("can't set non block to output pipe");
    }
}
