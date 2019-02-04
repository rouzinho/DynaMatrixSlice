#include "plugin.h"
#include "MatrixSlice.h"
#include <cedar/processing/ElementDeclaration.h>

void pluginDeclaration(cedar::aux::PluginDeclarationListPtr plugin)
{
    cedar::proc::ElementDeclarationPtr summation_decl
    (
        new cedar::proc::ElementDeclarationTemplate <MatrixSlice>("Utilities")
    );
    plugin->add(summation_decl);
}
