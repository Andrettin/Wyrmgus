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
	database::process_sml_scope_for_object(this, scope);
}

QVariantList module::get_dependencies_qvariant_list() const
{
	return container::to_qvariant_list(this->dependencies);
}

}
