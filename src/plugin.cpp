#include <exception>

#include <rofi/mode.h>
#include <rofi/mode-private.h>

#include "proxy.h"


static _mode_preprocess_input  combiOriginPreprocessInput = nullptr;

extern "C" {
extern Mode* rofi_collect_modi_search(const char *name);
}

static int ProxyInit(Mode *sw);
static void ProxyDestroy(Mode *sw);
static unsigned int ProxyGetNumEntries(const Mode *sw);
static ModeMode ProxyResult(Mode *sw, int mretv, char **input, unsigned int selectedLine);
static int ProxyTokenMatch(const Mode *sw, rofi_int_matcher **tokens, unsigned int index);
static char* ProxyGetDisplayValue(const Mode *sw, unsigned int selectedLine, int *state, GList **attrList, int getEntry);
static char* ProxyPreprocessInput(Mode *sw, const char *input);

Mode mode = {
    .abi_version        = ABI_VERSION,
    .name               = const_cast<char*>("proxy"),
    .cfg_name_key       =  {'d','i','s','p','l','a','y','-','p','r','o','x','y', 0},
    .display_name       = nullptr,
    ._init              = ProxyInit,
    ._destroy           = ProxyDestroy,
    ._get_num_entries   = ProxyGetNumEntries,
    ._result            = ProxyResult,
    ._token_match       = ProxyTokenMatch,
    ._get_display_value = ProxyGetDisplayValue,
    ._get_icon          = nullptr,
    ._get_completion    = nullptr,
    ._preprocess_input  = ProxyPreprocessInput,
    ._get_message       = nullptr,
    .private_data       = reinterpret_cast<void *>(new Proxy()),
    .free               = nullptr,
    .ed                 = nullptr,
    .module             = nullptr,
};

static Proxy* GetProxy(Mode *sw) {
    return reinterpret_cast<Proxy *>(mode_get_private_data(sw));
}

static const Proxy* GetProxy(const Mode *sw) {
    return reinterpret_cast<const Proxy *>(mode_get_private_data(sw));
}

static void logException(const char* func, const std::exception& e) {
    fprintf(stderr, "%s: finished with error: %s\n", func, e.what());
    fflush(stderr);
}

static int ProxyInit(Mode *sw) {
    try {
        GetProxy(sw)->Init();
        Mode* modeCombi = rofi_collect_modi_search("combi");
        if ((modeCombi != nullptr) && (modeCombi->private_data != nullptr)) {
            combiOriginPreprocessInput = modeCombi->_preprocess_input;
            modeCombi->_preprocess_input = ProxyPreprocessInput;
        }

        return TRUE;
    } catch(const std::exception& e) {
        logException("ProxyInit", e);
        return FALSE;
    }
}

static void ProxyDestroy(Mode *sw) {
    try {
        auto* proxy = GetProxy(sw);
        proxy->Destroy();
        delete proxy;
        mode_set_private_data(sw, nullptr);
    } catch(const std::exception& e) {
        logException("ProxyDestroy", e);
    }
}

static unsigned int ProxyGetNumEntries(const Mode *sw) {
    try {
        return static_cast<unsigned int>(GetProxy(sw)->GetLinesCount());
    } catch(const std::exception& e) {
        logException("ProxyGetNumEntries", e);
        return 0;
    }
}

static ModeMode ProxyResult(Mode */* sw */, int mretv, char **/* input */, unsigned int /* selectedLine */) {
    ModeMode retv = MODE_EXIT;

    if (mretv & MENU_NEXT) {
        retv = NEXT_DIALOG;
    } else if(mretv & MENU_PREVIOUS) {
        retv = PREVIOUS_DIALOG;
    } else if (mretv & MENU_QUICK_SWITCH) {
        retv = static_cast<ModeMode>(mretv & MENU_LOWER_MASK);
    } else if ((mretv & MENU_OK)) {
        retv = RELOAD_DIALOG;
    } else if ((mretv & MENU_ENTRY_DELETE) == MENU_ENTRY_DELETE) {
        retv = RELOAD_DIALOG;
    }

    return retv;
}

static int ProxyTokenMatch(const Mode *sw, rofi_int_matcher **tokens, unsigned int index) {
    try {
        return GetProxy(sw)->TokenMatch(tokens, static_cast<size_t>(index)) ? TRUE : FALSE;
    } catch(const std::exception& e) {
        logException("ProxyTokenMatch", e);
        return FALSE;
    }
}

static char* ProxyGetDisplayValue(const Mode *sw, unsigned int selectedLine, int */* state */, GList **/* attrList */, int getEntry) {
    if (!getEntry) {
        return nullptr;
    }

    try {
        return g_strdup(GetProxy(sw)->GetLine(static_cast<size_t>(selectedLine)));
    } catch(const std::exception& e) {
        logException("ProxyGetDisplayValue", e);
        return nullptr;
    }
}

static char* ProxyPreprocessInput(Mode *sw, const char *input) {
    try {
        if (combiOriginPreprocessInput != nullptr) {
            input = GetProxy(&mode)->PreprocessInput(input);
            return combiOriginPreprocessInput(sw, input);
        } else {
            return g_strdup(GetProxy(sw)->PreprocessInput(input));
        }
    } catch(const std::exception& e) {
        logException("ProxyPreprocessInput", e);
        return nullptr;
    }
}
