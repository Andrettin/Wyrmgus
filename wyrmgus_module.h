#ifndef __WYRMGUS_MODULE_H__
#define __WYRMGUS_MODULE_H__

#include "core/reference.h"

class WyrmgusModule : public Reference {
	GDCLASS(WyrmgusModule, Reference)

public:
	int Run();

protected:
	static void _bind_methods();
};

#endif
