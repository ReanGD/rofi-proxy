#include "process.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <glib.h>
#pragma GCC diagnostic pop
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>


namespace {

static void onProcessExit(GPid pid, gint status, gpointer context) {
    fprintf(stderr, "Child %" G_PID_FORMAT " exited %s\n", pid, g_spawn_check_exit_status (status, NULL) ? "normally" : "abnormally");
    if (context != nullptr) {
        auto* process = reinterpret_cast<Process*>(context);
        process->Destroy();
    }
    g_spawn_close_pid(pid);
    exit(0);
}

static int onProcessInput(GIOChannel */* source */, GIOCondition /* condition */, gpointer /* context */) {
    return G_SOURCE_CONTINUE;
}

}

Process::Process()
    : m_readFd(STDIN_FILENO)
    , m_writeFd(STDOUT_FILENO) {
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
    m_readChWatcher = g_io_add_watch(m_readCh, G_IO_IN, onProcessInput, nullptr);
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
