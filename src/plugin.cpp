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
    ._get_icon          = nullptr,
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

static const Proxy* GetProxy(const Mode* sw) {
    return reinterpret_cast<const Proxy *>(mode_get_private_data(sw));
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
        proxy->Destroy();
        delete proxy;
        mode_set_private_data(sw, nullptr);
    } catch(const std::exception& e) {
        logException("ProxyDestroy", e);
    }
}

static unsigned int ProxyGetNumEntries(const Mode* sw) {
    try {
        return static_cast<unsigned int>(GetProxy(sw)->GetLinesCount());
    } catch(const std::exception& e) {
        logException("ProxyGetNumEntries", e);
        return 0;
    }
}

static ModeMode ProxyResult(Mode* sw, int mretv, char**, unsigned int selectedLine) {
    ModeMode retv = MODE_EXIT;

    if (mretv & MENU_OK) {
        try {
            GetProxy(sw)->OnSelectLine(selectedLine);
        } catch(const std::exception& e) {
            logException("ProxyResult (", e);
        }
        return RELOAD_DIALOG;
    }

    if (mretv & MENU_NEXT) {
        retv = NEXT_DIALOG;
    } else if(mretv & MENU_PREVIOUS) {
        retv = PREVIOUS_DIALOG;
    } else if (mretv & MENU_QUICK_SWITCH) {
        retv = static_cast<ModeMode>(mretv & MENU_LOWER_MASK);
    } else if ((mretv & MENU_ENTRY_DELETE) == MENU_ENTRY_DELETE) {
        retv = RELOAD_DIALOG;
    }

    return retv;
}

static int ProxyTokenMatch(const Mode* sw, rofi_int_matcher** tokens, unsigned int index) {
    try {
        return GetProxy(sw)->TokenMatch(tokens, index) ? TRUE : FALSE;
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

static char* ProxyPreprocessInput(Mode *sw, const char *input) {
    try {
        return g_strdup(GetProxy(&mode)->PreprocessInput(sw, input));
    } catch(const std::exception& e) {
        logException("ProxyPreprocessInput", e);
        return nullptr;
    }
}

static char* ProxyGetHelpMessage(const Mode *sw) {
    try {
        const char* text = GetProxy(sw)->GetHelpMessage();
        return text == nullptr ? nullptr : g_strdup(text);
    } catch(const std::exception& e) {
        logException("ProxyGetHelpMessage", e);
        return nullptr;
    }
}
