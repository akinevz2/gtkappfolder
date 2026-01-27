#include "UIInterface.h"

#ifdef USE_GTK4
#include "GTK4UI.h"
#else
#include "GTK3UI.h"
#endif

namespace UI {

UIInterface* create_ui() {
#ifdef USE_GTK4
    return new GTK4UI();
#else
    return new GTK3UI();
#endif
}

} // namespace UI
