#include <exception>

#include <rofi/mode.h>
#include <rofi/mode-private.h>

#include "proxy.h"


static int ProxyInit(Mode* sw);
static void ProxyDestroy(Mode* sw);
static unsigned int ProxyGetNumEntries(const Mode* sw);
static ModeMode ProxyResult(Mode* sw, int mretv, char** input, unsigned int selectedLine);
static int ProxyTokenMatch(const Mode *sw, rofi_int_matcher** tokens, unsigned int index);
static char* ProxyGetDisplayValue(const Mode* sw, unsigned int selectedLine, int* state, GList** attrList, int getEntry);
static cairo_surface_t* GetIcon(const Mode* sw, unsigned int selectedLine, int height);
static char* ProxyPreprocessInput(Mode* sw, const char* input);
static char* ProxyGetHelpMessage(const Mode *sw);

Mode mode = {
    .abi_version        = ABI_VERSION,
    .name               = g_strdup("proxy"),
    .cfg_name_key       =  {'d','i','s','p','l','a','y','-','p','r','o','x','y', 0},
    .display_name       = g_strdup("proxy"),
    ._init              = ProxyInit,
    ._destroy           = ProxyDestroy,
    ._get_num_entries   = ProxyGetNumEntries,
    ._result            = ProxyResult,
    ._token_match       = ProxyTokenMatch,
    ._get_display_value = ProxyGetDisplayValue,
    ._get_icon          = GetIcon,
    ._get_completion    = nullptr,
    ._preprocess_input  = ProxyPreprocessInput,
    ._get_message       = ProxyGetHelpMessage,
    .private_data       = reinterpret_cast<void *>(new Proxy()),
    .free               = nullptr,
    .ed                 = nullptr,
    .module             = nullptr,
};

static Proxy* GetProxy(Mode* sw) {
    return reinterpret_cast<Proxy *>(mode_get_private_data(sw));
}

static void logException(const char* func, const std::exception& e) {
    fprintf(stderr, "%s: finished with error: %s\n", func, e.what());
    fflush(stderr);
}

static int ProxyInit(Mode* sw) {
    try {
        GetProxy(sw)->Init(sw);
        return TRUE;
    } catch(const std::exception& e) {
        logException("ProxyInit", e);
        return FALSE;
    }
}

static void ProxyDestroy(Mode* sw) {
    try {
        auto* proxy = GetProxy(sw);
        if (proxy != nullptr) {
            mode_set_private_data(sw, nullptr);
            proxy->Destroy();
            delete proxy;
        }
    } catch(const std::exception& e) {
        logException("ProxyDestroy", e);
    }
}

static unsigned int ProxyGetNumEntries(const Mode*) {
    try {
        return static_cast<unsigned int>(GetProxy(&mode)->GetLinesCount());
    } catch(const std::exception& e) {
        logException("ProxyGetNumEntries", e);
        return 0;
    }
}

static ModeMode ProxyResult(Mode*, int mretv, char** input, unsigned int selectedLine) {
    if (mretv & MENU_NEXT) {
        return NEXT_DIALOG;
    }

    if (mretv & MENU_PREVIOUS) {
        return PREVIOUS_DIALOG;
    }

    if (mretv & MENU_OK) {
        try {
            GetProxy(&mode)->OnSelectLine(selectedLine);
        } catch(const std::exception& e) {
            logException("ProxyResult (MENU_OK)", e);
        }
        return RELOAD_DIALOG;
    }

    if (mretv & MENU_ENTRY_DELETE) {
        try {
            GetProxy(&mode)->OnDeleteLine(selectedLine);
        } catch(const std::exception& e) {
            logException("ProxyResult (MENU_ENTRY_DELETE)", e);
        }
        return RELOAD_DIALOG;
    }

    if (mretv & MENU_CUSTOM_INPUT) {
        try {
            GetProxy(&mode)->OnSelectCustomInput(*input);
        } catch(const std::exception& e) {
            logException("ProxyResult (MENU_CUSTOM_INPUT)", e);
        }
        return RELOAD_DIALOG;
    }

    if (mretv & MENU_CANCEL) {
        try {
            if (bool exit = GetProxy(&mode)->OnCancel(); !exit) {
                return RELOAD_DIALOG;
            }
        } catch(const std::exception& e) {
            logException("ProxyResult (MENU_CANCEL)", e);
        }
        return MODE_EXIT;
    }

    if (mretv & MENU_QUICK_SWITCH) {
        try {
            GetProxy(&mode)->OnCustomKey(selectedLine, ((mretv & MENU_LOWER_MASK) % 20) + 1);
        } catch(const std::exception& e) {
            logException("ProxyResult (MENU_QUICK_SWITCH)", e);
        }
        return static_cast<ModeMode>(mretv & MENU_LOWER_MASK);
    }

    // MENU_CUSTOM_ACTION

    return MODE_EXIT;
}

static int ProxyTokenMatch(const Mode*, rofi_int_matcher** tokens, unsigned int index) {
    try {
        return GetProxy(&mode)->OnLineMatch(tokens, index) ? TRUE : FALSE;
    } catch(const std::exception& e) {
        logException("ProxyTokenMatch", e);
        return FALSE;
    }
}

static char* ProxyGetDisplayValue(const Mode*, unsigned int selectedLine, int* state, GList**, int getEntry) {
    try {
        const char* text = GetProxy(&mode)->GetLine(selectedLine, state);
        return getEntry ? g_strdup(text) : nullptr;
    } catch(const std::exception& e) {
        logException("ProxyGetDisplayValue", e);
        return nullptr;
    }
}

static cairo_surface_t* GetIcon(const Mode*, unsigned int selectedLine, int height) {
    try {
        return GetProxy(&mode)->GetIcon(selectedLine, height);
    } catch(const std::exception& e) {
        logException("GetIcon", e);
        return nullptr;
    }
}

static char* ProxyPreprocessInput(Mode* sw, const char *input) {
    try {
        return g_strdup(GetProxy(&mode)->OnInput(sw, input));
    } catch(const std::exception& e) {
        logException("ProxyPreprocessInput", e);
        return nullptr;
    }
}

static char* ProxyGetHelpMessage(const Mode*) {
    try {
        const char* text = GetProxy(&mode)->GetHelpMessage();
        return text == nullptr ? nullptr : g_strdup(text);
    } catch(const std::exception& e) {
        logException("ProxyGetHelpMessage", e);
        return nullptr;
    }
}
