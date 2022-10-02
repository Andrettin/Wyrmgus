#pragma once

namespace wyrmgus {

class enum_converter_base
{
protected:
	static void register_string_to_qvariant_conversion(const std::string &class_name, std::function<QVariant(const std::string &)> &&function);
};

template <typename enum_type>
class enum_converter final : public enum_converter_base
{
private:
	enum_converter()
	{
		//this check is required for class_initialized variable and, correspondingly,
		//the data_type::initialize_class() call to not to be initialized away
		if (!enum_converter::initialized) {
			throw std::runtime_error("Never reached.");
		}
	}

public:
	static enum_type to_enum(const std::string &str)
	{
		const auto find_iterator = enum_converter::string_to_enum_map.find(str);

		if (find_iterator != enum_converter::string_to_enum_map.end()) {
			return find_iterator->second;
		}

		throw std::runtime_error("Invalid enum string: \"" + str + "\".");
	}

	static const std::string &to_string(const enum_type enum_value)
	{
		const auto find_iterator = enum_converter::enum_to_string_map.find(enum_value);

		if (find_iterator != enum_converter::enum_to_string_map.end()) {
			return find_iterator->second;
		}

		throw std::runtime_error("Invalid enum value: " + std::to_string(static_cast<std::underlying_type_t<enum_type>>(enum_value)) + ".");
	}

private:
	static bool initialize()
	{
		enum_converter::initialize_enum_to_string_map();

		enum_converter_base::register_string_to_qvariant_conversion(enum_converter::property_class_identifier, [](const std::string &value) {
			return QVariant::fromValue(enum_converter::to_enum(value));
		});

		return true;
	}

	static void initialize_enum_to_string_map()
	{
		for (const auto &[enum_str, enum_value] : enum_converter::string_to_enum_map) {
			enum_converter::enum_to_string_map[enum_value] = enum_str;
		}
	}

private:
	static const std::string property_class_identifier;
	static const std::map<std::string, enum_type> string_to_enum_map; //this should be defined where the enum converter is specialized
	static inline std::map<enum_type, std::string> enum_to_string_map;
	static const bool initialized;
};

}
