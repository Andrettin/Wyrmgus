#ifndef __WYRMGUS_MODULE_H__
#define __WYRMGUS_MODULE_H__

#include <core/reference.h>

class CCampaign;

class WyrmgusModule : public Reference
{
	GDCLASS(WyrmgusModule, Reference)

public:
	int Run();
	String GetVersion();
	//Vector<CCampaign *> GetCampaigns();

protected:
	static void _bind_methods();
};

#endif
