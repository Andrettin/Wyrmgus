//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
/**@name property.h - The property header file. */
//
//      (c) Copyright 2019 by Andrettin
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//

#ifndef __PROPERTY_H__
#define __PROPERTY_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_type.h"
#include "type_traits.h"

#include <core/ustring.h>
#include <core/variant.h>

#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class DataElement;

extern std::string FindAndReplaceString(const std::string &text, const std::string &find, const std::string &replace);
extern bool StringToBool(const std::string &str);

template <typename T>
Array VectorToGodotArray(const std::vector<T> &vector);

template <typename T>
extern T ConvertFromString(const std::string &str);

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class PropertyCommonBase
{
public:
	virtual ~PropertyCommonBase() {}
	
	virtual Variant ToVariant() const = 0;
	
protected:
	virtual const PropertyCommonBase &operator =(const std::string &rhs) = 0;
	virtual const PropertyCommonBase &operator +=(const std::string &rhs) = 0;
	virtual const PropertyCommonBase &operator -=(const std::string &rhs) = 0;
	virtual const PropertyCommonBase &operator *=(const std::string &rhs) = 0;
	virtual const PropertyCommonBase &operator /=(const std::string &rhs) = 0;
	
	friend DataElement;
};

template <typename T>
class PropertyBase : public PropertyCommonBase
{
public:
	using ValueType = T;
	using ReturnType = std::conditional_t<std::disjunction_v<std::is_fundamental<T>, std::is_pointer<T>>, const T, const T &>;
	using ModifiableReturnType = std::conditional_t<std::disjunction_v<std::is_fundamental<T>, std::is_pointer<T>>, T, T &>;
	using PointerType = std::conditional_t<std::is_pointer_v<T>, T, const std::add_pointer_t<T>>;
	using ModifiablePointerType = std::conditional_t<std::is_pointer_v<T>, T, std::add_pointer_t<T>>;
	using ArgumentType = const T &;
	using GetterType = std::function<ReturnType()>;
	using SetterType = std::function<void(ArgumentType)>;
	
protected:
	PropertyBase(const T &value, const GetterType &getter, const SetterType &setter) : Getter(getter), Setter(setter)
	{
		this->Value = new T;
		*(this->Value) = value;
	}
	
	PropertyBase(const GetterType &getter, const SetterType &setter) : Getter(getter), Setter(setter)
	{
	}
	
	virtual ~PropertyBase()
	{
		if (this->Value) {
			delete this->Value;
		}
	}

public:
	virtual Variant ToVariant() const override
	{
		if constexpr(is_specialization_of<T, std::vector>::value) {
			return VectorToGodotArray(this->Get());
		} else {
			return Variant(this->Get());
		}
	}

public:	
	const PropertyBase<T> &operator =(const T &rhs)
	{
		this->Set(rhs);
		return *this;
	}
	
	const PropertyBase<T> &operator =(const PropertyBase<T> &rhs)
	{
		return *this = rhs.Get();
	}
	
	/**
	**	@brief	The operator for assignment from a string for a property
	**
	**	@param	rhs	The string which has to be converted to the property
	**
	**	@return	The property
	*/
	virtual const PropertyBase<T> &operator =(const std::string &rhs) override
	{
		if constexpr(
			std::is_same_v<T, int>
			|| std::is_same_v<T, bool>
			|| std::is_same_v<T, String>
			|| std::is_pointer_v<T>
		) {
			return *this = ConvertFromString<T>(rhs);
		} else {
			fprintf(stderr, "The operator = is not valid for this type.\n");
			return *this;
		}
	}
	
	const PropertyBase<T> &operator +=(const T &rhs)
	{
		this->Set(this->Get() + rhs);
		return *this;
	}
	
	const PropertyBase<T> &operator +=(const PropertyBase<T> &rhs)
	{
		return *this += rhs.Get();
	}
	
	/**
	**	@brief	The operator for addition of a string for a property
	**
	**	@param	rhs	The string which has to be converted for the property
	**
	**	@return	The property
	*/
	virtual const PropertyBase<T> &operator +=(const std::string &rhs) override
	{
		if constexpr(
			std::is_same_v<T, int>
			|| std::is_same_v<T, bool>
			|| std::is_same_v<T, String>
		) {
			return *this += ConvertFromString<T>(rhs);
		} else if constexpr(is_specialization_of<T, std::vector>::value) {
			typename T::value_type new_value = ConvertFromString<typename T::value_type>(rhs);
			this->GetModifiable().push_back(new_value);
			return *this;
		} else {
			fprintf(stderr, "The operator += is not valid for this type.\n");
			return *this;
		}
	}

	const PropertyBase<T> &operator -=(const T &rhs)
	{
		this->Set(this->Get() - rhs);
		return *this;
	}
	
	const PropertyBase<T> &operator -=(const PropertyBase<T> &rhs)
	{
		return *this -= rhs.Get();
	}
	
	/**
	**	@brief	The operator for subtraction of a string for a property
	**
	**	@param	rhs	The string which has to be converted for the property
	**
	**	@return	The property
	*/
	virtual const PropertyBase<T> &operator -=(const std::string &rhs) override
	{
		if constexpr(
			std::is_same_v<T, int>
			|| std::is_same_v<T, bool>
		) {
			return *this -= ConvertFromString<T>(rhs);
		} else if constexpr(is_specialization_of<T, std::vector>::value) {
			typename T::value_type new_value = ConvertFromString<typename T::value_type>(rhs);
			this->GetModifiable().erase(std::remove(this->GetModifiable().begin(), this->GetModifiable().end(), new_value), this->GetModifiable().end());
			return *this;
		} else {
			fprintf(stderr, "The operator -= is not valid for this type.\n");
			return *this;
		}
	}
	
	const PropertyBase<T> &operator *=(const T &rhs)
	{
		this->Set(this->Get() * rhs);
		return *this;
	}
	
	const PropertyBase<T> &operator *=(const PropertyBase<T> &rhs)
	{
		return *this *= rhs.Get();
	}
	
	virtual const PropertyBase<T> &operator *=(const std::string &rhs) override
	{
		if constexpr(std::is_same_v<T, int>) {
			return *this *= std::stoi(rhs);
		} else if constexpr(std::is_same_v<T, bool>) {
			return *this *= StringToBool(rhs);
		} else {
			fprintf(stderr, "The operator *= is not valid for this type.");
			return *this;
		}
	}

	const PropertyBase<T> &operator /=(const T &rhs)
	{
		this->Set(this->Get() / rhs);
		return *this;
	}
	
	const PropertyBase<T> &operator /=(const PropertyBase<T> &rhs)
	{
		return *this /= rhs.Get();
	}
	
	virtual const PropertyBase<T> &operator /=(const std::string &rhs) override
	{
		if constexpr(std::is_same_v<T, int>) {
			return *this /= std::stoi(rhs);
		} else {
			fprintf(stderr, "The operator /= is not valid for this type.");
			return *this;
		}
	}

	ReturnType Get() const
	{
		//get the underlying value
		if (this->Getter) {
			return this->Getter();
		} else {
			return *this->Value;
		}
	}
	
	void Set(const T &value)
	{
		if (this->Setter) {
			this->Setter(value);
		} else if (this->Value != nullptr && *this->Value != value) {
			*this->Value = value;
		}
	}
	
protected:
	ModifiableReturnType GetModifiable() const
	{
		if constexpr(std::is_reference_v<ReturnType>) {
			return const_cast<ModifiableReturnType>(this->Get());
		} else {
			return this->Get();
		}
	}
	
	T *Value = nullptr;
	GetterType Getter;
	SetterType Setter;
};

template <typename T>
class ProtectedPropertyBase : protected PropertyBase<T>
{
protected:
	ProtectedPropertyBase(const T &value, const GetterType &getter, const SetterType &setter) : PropertyBase(value, getter, setter)
	{
	}
	
	ProtectedPropertyBase(const GetterType &getter, const SetterType &setter) : PropertyBase(getter, setter)
	{
	}
	
public:
	ReturnType Get() const
	{
		//get the underlying value
		return PropertyBase<T>::Get();
	}
	
	PointerType operator ->() const
	{
		if constexpr(std::is_pointer_v<T>) {
			return this->Get();
		} else if constexpr(!is_fundamental_v<T>) {
			return &this->Get();
		} else {
			return nullptr;
		}
	}
	
	operator ReturnType() const
	{
		return this->Get();
	}
	
	bool operator ==(const T &rhs) const
	{
		return this->Get() == rhs;
	}
	
	bool operator ==(const PropertyBase<T> &rhs) const
	{
		return *this == rhs.Get();
	}
	
	bool operator !=(const T &rhs) const
	{
		return !(*this == rhs);
	}
	
	bool operator !=(const PropertyBase<T> &rhs) const
	{
		return *this != rhs.Get();
	}
	
	bool operator <(const T &rhs) const
	{
		return this->Get() < rhs;
	}
	
	bool operator <(const PropertyBase<T> &rhs) const
	{
		return *this < rhs.Get();
	}
	
	T operator +(const T &rhs)
	{
		return this->Get() + rhs;
	}
	
	T operator +(const PropertyBase<T> &rhs)
	{
		return *this + rhs.Get();
	}
	
	T operator -(const T &rhs)
	{
		return this->Get() - rhs;
	}
	
	T operator -(const PropertyBase<T> &rhs)
	{
		return *this - rhs.Get();
	}
	
	T operator *(const T &rhs)
	{
		return this->Get() * rhs;
	}
	
	T operator *(const PropertyBase<T> &rhs)
	{
		return *this * rhs.Get();
	}
	
	T operator /(const T &rhs)
	{
		return this->Get() / rhs;
	}
	
	T operator /(const PropertyBase<T> &rhs)
	{
		return *this / rhs.Get();
	}
};

template <typename T, typename O>
class Property : public ProtectedPropertyBase<T>
{
private:
	Property(const T &value, const GetterType &getter, const SetterType &setter = nullptr) : ProtectedPropertyBase(value, getter, setter)
	{
	}
	
	Property(const T &value, const SetterType &setter = nullptr) : Property(value, nullptr, setter)
	{
	}
	
	Property(const GetterType &getter, const SetterType &setter = nullptr) : ProtectedPropertyBase(getter, setter)
	{
	}
	
	Property(const SetterType &setter = nullptr) : Property(T(), nullptr, setter)
	{
	}
	
	friend O;
};

template <typename O>
class Property<String, O> : public ProtectedPropertyBase<String>
{
private:
	Property(const String &value, const GetterType &getter, const SetterType &setter = nullptr) : ProtectedPropertyBase(value, getter, setter)
	{
	}
	
	Property(const String &value, const SetterType &setter = nullptr) : Property(value, nullptr, setter)
	{
	}
	
	Property(const GetterType &getter, const SetterType &setter = nullptr) : ProtectedPropertyBase(getter, setter)
	{
	}
	
	Property(const SetterType &setter = nullptr) : Property(String(), nullptr, setter)
	{
	}
	
public:
	bool empty() const
	{
		return this->Get().empty();
	}
	
	CharString utf8() const
	{
		return this->Get().utf8();
	}
	
	friend O;
};

template <typename T, typename O>
class Property<std::vector<T>, O> : public ProtectedPropertyBase<std::vector<T>>
{
	using ValueType = std::vector<T>;
	using ReturnType = const std::vector<T> &;
	using ArgumentType = const std::vector<T> &;
	using GetterType = std::function<ReturnType()>;
	using SetterType = std::function<void(ArgumentType)>;
	
private:
	Property(const std::vector<T> &value, const GetterType &getter, const SetterType &setter = nullptr) : ProtectedPropertyBase(value, getter, setter)
	{
	}
	
	Property(const std::vector<T> &value, const SetterType &setter = nullptr) : Property(value, nullptr, setter)
	{
	}
	
	Property(const GetterType &getter, const SetterType &setter = nullptr) : ProtectedPropertyBase(getter, setter)
	{
	}
	
	Property(const SetterType &setter = nullptr) : Property(std::vector<T>(), nullptr, setter)
	{
	}
	
public:
	bool empty() const
	{
		return this->Get().empty();
	}
	
	typename std::vector<T>::const_iterator begin() const
	{
		return this->Get().begin();
	}
	
	typename std::vector<T>::const_iterator end() const
	{
		return this->Get().end();
	}
	
private:
	typename std::vector<T>::iterator begin()
	{
		return this->GetModifiable().begin();
	}
	
	typename std::vector<T>::iterator end()
	{
		return this->GetModifiable().end();
	}
	
	friend O;
};

template <typename T>
class ExposedPropertyBase : public PropertyBase<T>
{
protected:
	ExposedPropertyBase(const T &value, const GetterType &getter, const SetterType &setter) : PropertyBase(getter, setter)
	{
		this->Value = new T;
		*(this->Value) = value;
	}
	
	ExposedPropertyBase(const GetterType &getter, const SetterType &setter) : PropertyBase(getter, setter)
	{
	}
	
public:
	ModifiableReturnType Get() const
	{
		//get the underlying value
		return PropertyBase<T>::GetModifiable();
	}
	
	ModifiablePointerType operator ->()
	{
		if constexpr(std::is_pointer_v<T>) {
			return this->Get();
		} else if constexpr(!is_fundamental_v<T>) {
			return &this->Get();
		} else {
			return nullptr;
		}
	}
	
	PointerType operator ->() const
	{
		if constexpr(std::is_pointer_v<T>) {
			return this->Get();
		} else if constexpr(!is_fundamental_v<T>) {
			return &this->Get();
		} else {
			return nullptr;
		}
	}
	
	operator ModifiableReturnType() const
	{
		return this->Get();
	}
	
	bool operator ==(const T &rhs) const
	{
		return this->Get() == rhs;
	}
	
	bool operator ==(const PropertyBase<T> &rhs) const
	{
		return *this == rhs.Get();
	}
	
	bool operator !=(const T &rhs) const
	{
		return !(*this == rhs);
	}
	
	bool operator !=(const PropertyBase<T> &rhs) const
	{
		return *this != rhs.Get();
	}
	
	bool operator <(const T &rhs) const
	{
		return this->Get() < rhs;
	}
	
	bool operator <(const PropertyBase<T> &rhs) const
	{
		return *this < rhs.Get();
	}
	
	T operator +(const T &rhs)
	{
		return this->Get() + rhs;
	}
	
	T operator +(const PropertyBase<T> &rhs)
	{
		return *this + rhs.Get();
	}
	
	T operator -(const T &rhs)
	{
		return this->Get() - rhs;
	}
	
	T operator -(const PropertyBase<T> &rhs)
	{
		return *this - rhs.Get();
	}
	
	T operator *(const T &rhs)
	{
		return this->Get() * rhs;
	}
	
	T operator *(const PropertyBase<T> &rhs)
	{
		return *this * rhs.Get();
	}
	
	T operator /(const T &rhs)
	{
		return this->Get() / rhs;
	}
	
	T operator /(const PropertyBase<T> &rhs)
	{
		return *this / rhs.Get();
	}
};

/**
**	@brief	A property which can be written to publicly
*/
template <typename T, typename O>
class ExposedProperty : public ExposedPropertyBase<T>
{
private:
	ExposedProperty(const T &value, const GetterType &getter, const SetterType &setter = nullptr) : ExposedPropertyBase(value, getter, setter)
	{
	}
	
	ExposedProperty(const T &value, const SetterType &setter = nullptr) : ExposedProperty(value, nullptr, setter)
	{
	}
	
	ExposedProperty(const GetterType &getter, const SetterType &setter = nullptr) : ExposedPropertyBase(getter, setter)
	{
	}
	
	ExposedProperty(const SetterType &setter = nullptr) : ExposedProperty(T(), nullptr, setter)
	{
	}
	
public:
	const ExposedPropertyBase<T> &operator =(const T &rhs)
	{
		this->Set(rhs);
		return *this;
	}
	
	friend O;
};

template <typename O>
class ExposedProperty<String, O> : public ExposedPropertyBase<String>
{
private:
	ExposedProperty(const String &value, const GetterType &getter, const SetterType &setter = nullptr) : ExposedPropertyBase(value, getter, setter)
	{
	}
	
	ExposedProperty(const String &value, const SetterType &setter = nullptr) : ExposedProperty(value, nullptr, setter)
	{
	}
	
	ExposedProperty(const GetterType &getter, const SetterType &setter = nullptr) : ExposedPropertyBase(getter, setter)
	{
	}
	
	ExposedProperty(const SetterType &setter = nullptr) : ExposedProperty(String(), nullptr, setter)
	{
	}
	
public:
	const ExposedPropertyBase<String> &operator =(const String &rhs)
	{
		this->Set(rhs);
		return *this;
	}
	
	const ExposedPropertyBase<String> &operator =(const char *rhs)
	{
		return *this = String(rhs);
	}
	
	const ExposedPropertyBase<String> &operator =(const std::string &rhs)
	{
		return *this = rhs.c_str();
	}
	
	bool empty() const
	{
		return this->Get().empty();
	}
	
	CharString utf8() const
	{
		return this->Get().utf8();
	}
	
	friend O;
};

template <typename T, typename O>
class ExposedProperty<std::vector<T>, O> : public ExposedPropertyBase<std::vector<T>>
{
	using ValueType = std::vector<T>;
	using ReturnType = const std::vector<T> &;
	using ArgumentType = const T &;
	using GetterType = std::function<ReturnType()>;
	using SetterType = std::function<void(ArgumentType)>;
	
private:
	ExposedProperty(const std::vector<T> &value, const GetterType &getter, const SetterType &setter = nullptr) : ExposedPropertyBase(value, getter, setter)
	{
	}
	
	ExposedProperty(const std::vector<T> &value, const SetterType &setter = nullptr) : ExposedProperty(value, nullptr, setter)
	{
	}
	
	ExposedProperty(const GetterType &getter, const SetterType &setter = nullptr) : ExposedPropertyBase(getter, setter)
	{
	}
	
	ExposedProperty(const SetterType &setter = nullptr) : ExposedProperty(std::vector<T>(), nullptr, setter)
	{
	}
	
public:
	bool empty() const
	{
		return this->Get().empty();
	}
	
	typename std::vector<T>::const_iterator begin() const
	{
		return this->Get().begin();
	}
	
	typename std::vector<T>::const_iterator end() const
	{
		return this->Get().end();
	}
	
	typename std::vector<T>::iterator begin()
	{
		return this->Get().begin();
	}
	
	typename std::vector<T>::iterator end()
	{
		return this->Get().end();
	}
	
	friend O;
};

template <typename T>
inline T operator + (const T &lhs, const PropertyBase<T> &rhs)
{
	return lhs + rhs.Get();
}

//overload std::min and std::max so that they work as expected with the properties
namespace std {
	//std::min
	template<class T>
	constexpr const T &min(const PropertyBase<T> &a, const T &b)
	{
		return min(a.Get(), b);
	}
	
	template<class T>
	constexpr const T &min(const T &a, const PropertyBase<T> &b)
	{
		return min(a, b.Get());
	}
	
	template<class T>
	constexpr const T &min(const PropertyBase<T> &a, const PropertyBase<T> &b)
	{
		return min(a.Get(), b.Get());
	}
	
	//std::max
	template<class T>
	constexpr const T &max(const PropertyBase<T> &a, const T &b)
	{
		return max(a.Get(), b);
	}
	
	template<class T>
	constexpr const T &max(const T &a, const PropertyBase<T> &b)
	{
		return max(a, b.Get());
	}
	
	template<class T>
	constexpr const T &max(const PropertyBase<T> &a, const PropertyBase<T> &b)
	{
		return max(a.Get(), b.Get());
	}
}

#endif
