#include "database/module.h"

#include "database/database.h"
#include "util/container_util.h"

namespace wyrmgus {

void module::process_sml_property(const sml_property &property)
{
	database::process_sml_property_for_object(this, property);
}

void module::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "dependencies") {
		for (const std::string &value : values) {
			this->add_dependency(database::get()->get_module(value));
		}
	} else {
		database::process_sml_scope_for_object(this, scope);
	}
}

}
