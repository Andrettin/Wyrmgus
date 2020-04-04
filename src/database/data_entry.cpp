#include "database/data_entry.h"

#include "database/database.h"

namespace stratagus {

void data_entry::process_sml_property(const sml_property &property)
{
	if (property.get_key() == "aliases") {
		return; //alias addition is already handled in the data type class
	}

	database::process_sml_property_for_object(this, property);
}

void data_entry::process_sml_scope(const sml_data &scope)
{
	database::process_sml_scope_for_object(this, scope);
}

}
