#include <iostream>
#include <exception>

#include <rofi/mode.h>
#include <rofi/mode-private.h>

#include "proxy.h"


static Proxy* GetProxy(Mode *sw) {
    return reinterpret_cast<Proxy *>(mode_get_private_data(sw));
}

static const Proxy* GetProxy(const Mode *sw) {
    return reinterpret_cast<const Proxy *>(mode_get_private_data(sw));
}

static int ProxyInit(Mode *sw) {
    if (mode_get_private_data(sw) == nullptr) {
        Proxy* proxy = nullptr;
        try {
            proxy = new Proxy();
            proxy->Init();
            mode_set_private_data(sw, reinterpret_cast<void *>(proxy));
        } catch(const std::exception& e) {
            if (proxy != nullptr) {
                delete proxy;
            }
            std::cerr << "ProxyInit: failed with error: " << e.what() << std::endl;
            return FALSE;
        }
    }

    return TRUE;
}

static void ProxyDestroy(Mode *sw) {
    auto* proxy = GetProxy(sw);
    if (proxy != nullptr) {
        try {
            delete proxy;
        } catch(const std::exception& e) {
            std::cerr << "ProxyDestroy: failed with error: " << e.what() << std::endl;
        }
        mode_set_private_data(sw, nullptr);
    }
}

static unsigned int ProxyGetNumEntries(const Mode *sw) {
    try {
        const auto* proxy = GetProxy(sw);
        return static_cast<unsigned int>(proxy->GetLinesCount());
    } catch(const std::exception& e) {
        std::cerr << "ProxyGetNumEntries: failed with error: " << e.what() << std::endl;
        return 0;
    }
}

static ModeMode ProxyResult(Mode */* sw */, int mretv, char **/* input */, unsigned int /* selected_line */) {
    ModeMode retv = MODE_EXIT;

    if (mretv & MENU_NEXT) {
        retv = NEXT_DIALOG;
    } else if(mretv & MENU_PREVIOUS) {
        retv = PREVIOUS_DIALOG;
    } else if (mretv & MENU_QUICK_SWITCH) {
        retv = static_cast<ModeMode>( mretv & MENU_LOWER_MASK);
    } else if ((mretv & MENU_OK)) {
        retv = RELOAD_DIALOG;
    } else if ((mretv & MENU_ENTRY_DELETE ) == MENU_ENTRY_DELETE) {
        retv = RELOAD_DIALOG;
    }

    return retv;
}

static int ProxyTokenMatch(const Mode *sw, rofi_int_matcher **tokens, unsigned int index) {
    try {
        const auto* proxy = GetProxy(sw);
        return proxy->TokenMatch(tokens, static_cast<size_t>(index)) ? TRUE : FALSE;
    } catch(const std::exception& e) {
        std::cerr << "ProxyTokenMatch: failed with error: " << e.what() << std::endl;
        return FALSE;
    }
}

static char* ProxyGetDisplayValue(const Mode *sw, unsigned int selectedLine, G_GNUC_UNUSED int */* state */, G_GNUC_UNUSED GList **/* attrList */, int getEntry) {
    if (!getEntry) {
        return nullptr;
    }

    try {
        const auto* proxy = GetProxy(sw);
        return g_strdup(proxy->GetLine(static_cast<size_t>(selectedLine)));
    } catch(const std::exception& e) {
        std::cerr << "ProxyGetDisplayValue: failed with error: " << e.what() << std::endl;
        return nullptr;
    }
}

Mode mode = {
    .abi_version        = ABI_VERSION,
    .name               = const_cast<char*>("proxy"),
    .cfg_name_key       =  {'d','i','s','p','l','a','y','-','p','r','o','x','y', 0},
    .display_name       = NULL,
    ._init              = ProxyInit,
    ._destroy           = ProxyDestroy,
    ._get_num_entries   = ProxyGetNumEntries,
    ._result            = ProxyResult,
    ._token_match       = ProxyTokenMatch,
    ._get_display_value = ProxyGetDisplayValue,
    ._get_icon          = NULL,
    ._get_completion    = NULL,
    ._preprocess_input  = NULL,
    ._get_message       = NULL,
    .private_data       = NULL,
    .free               = NULL,
    .ed                 = NULL,
};
