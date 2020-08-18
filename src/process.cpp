#include "process.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <glib.h>
#pragma GCC diagnostic pop

#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>

#include "proxy_base.h"


namespace {

static void onProcessExit(GPid pid, gint status, gpointer context) {
    fprintf(stderr, "Child %" G_PID_FORMAT " exited %s\n", pid, g_spawn_check_exit_status(status, nullptr) ? "normally" : "abnormally");
    if (context != nullptr) {
        auto* process = reinterpret_cast<Process*>(context);
        process->Destroy();
    }
    g_spawn_close_pid(pid);
    exit(0);
}

static int onProcessInput(GIOChannel *source, GIOCondition /* condition */, gpointer context) {
    static GString* buffer = g_string_sized_new(1024);

    gunichar unichar;
    GError* error = nullptr;
    GIOStatus status = g_io_channel_read_unichar(source, &unichar, &error);

    //when there is nothing to read, status is G_IO_STATUS_AGAIN
    while (status == G_IO_STATUS_NORMAL) {
        g_string_append_unichar(buffer, unichar);
        if (unichar == '\n') {
            if (buffer->len > 1) { //input is not an empty line
                if (context != nullptr) {
                    auto* process = reinterpret_cast<Process*>(context);
                    process->OnNewLine(buffer->str);
                }
            }
            g_string_set_size(buffer, 0);
        }
        status = g_io_channel_read_unichar(source, &unichar, &error);
    }

    if (status == G_IO_STATUS_ERROR) {
        fprintf(stderr, "Child process stdout error: %s\n", error->message);
        g_error_free(error);
    }

    return G_SOURCE_CONTINUE;
}

}

Process::Process(ProxyBase* parent)
    : m_readFd(STDIN_FILENO)
    , m_writeFd(STDOUT_FILENO)
    , m_parent(parent) {
}

Process::~Process() {
    if(m_pid != 0){
        kill(m_pid, SIGTERM);
    }
    Destroy();
}

void Process::Create(char **argv) {
    if (argv != nullptr) {
        Spawn(argv);
    }

    SetNonBlockFlag(m_readFd);
    m_readCh = g_io_channel_unix_new(m_readFd);
    m_writeCh = g_io_channel_unix_new(m_writeFd);
    m_readChWatcher = g_io_add_watch(m_readCh, G_IO_IN, onProcessInput, this);
}

void Process::Destroy() {
    m_pid = 0;

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
}

void Process::OnNewLine(const char* text) {
    if (m_parent != nullptr) {
        m_parent->OnNewLine(text);
    }
}

void Process::Spawn(char **argv) {
    char **envp = nullptr;
    GError *error = nullptr;
    void* userData = nullptr;
    int* errorFDPtr = nullptr;
    const char* workingDirectory = nullptr;
    GSpawnChildSetupFunc childSetup = nullptr;
    GSpawnFlags flags = static_cast<GSpawnFlags>(G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH);

    if (g_spawn_async_with_pipes(workingDirectory, argv, envp, flags, childSetup, userData,
                                &m_pid, &m_writeFd, &m_readFd, errorFDPtr, &error) == FALSE) {
        auto msg = std::string(error->message);
        g_error_free(error);
        throw std::runtime_error(msg);
    }

    g_child_watch_add(m_pid, onProcessExit, this);
}

void Process::SetNonBlockFlag(int fd) {
    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) != 0) {
        throw std::runtime_error("can't set non block to output pipe");
    }
}
