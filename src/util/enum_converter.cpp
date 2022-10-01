#include "util/enum_converter.h"

#include "database/database.h"

namespace wyrmgus {

void enum_converter_base::register_string_to_qvariant_conversion(const std::string &class_name, std::function<QVariant(const std::string &)> &&function)
{
	database::get()->register_string_to_qvariant_conversion(class_name, std::move(function));
}

}
