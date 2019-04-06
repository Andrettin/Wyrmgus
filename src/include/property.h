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

#include <core/ustring.h>
#include <core/variant.h>

#include <functional>
#include <stdexcept>
#include <string>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class DataElement;

extern std::string FindAndReplaceString(const std::string &text, const std::string &find, const std::string &replace);
extern bool StringToBool(const std::string &str);

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class PropertyCommonBase
{
public:
	virtual ~PropertyCommonBase() {}
	
protected:
	virtual const PropertyCommonBase &operator =(const std::string &rhs) = 0;
	virtual const PropertyCommonBase &operator +=(const std::string &rhs) = 0;
	virtual const PropertyCommonBase &operator -=(const std::string &rhs) = 0;
	virtual const PropertyCommonBase &operator *=(const std::string &rhs) = 0;
	virtual const PropertyCommonBase &operator /=(const std::string &rhs) = 0;
	
	virtual operator Variant() const = 0;
	
	friend DataElement;
};

template <typename T>
class PropertyBase : public PropertyCommonBase
{
public:
	using ValueType = T;
	using ReturnType = std::conditional_t<std::is_same_v<T, String>, const T &, T>;
	using GetterType = std::function<ReturnType()>;
	using SetterType = std::function<void(const T &)>;
	
protected:
	PropertyBase(const T &value, const GetterType &getter, const SetterType &setter) : Getter(getter), Setter(setter)
	{
		this->Value = new T;
		*(this->Value) = value;
	}
	
	PropertyBase(const GetterType &getter, const SetterType &setter) : Getter(getter), Setter(setter)
	{
	}
	
public:
	virtual ~PropertyBase()
	{
		if (this->Value) {
			delete this->Value;
		}
	}

	const T &operator *() const
	{
		if (this->Value == nullptr) {
			throw std::runtime_error("Tried to get the underlying value from a property which has none.");
		}
		
		//get the underlying value
		return *this->Value;
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
	
	ReturnType operator ->()
	{
		return this->Get();
	}
	
	operator ReturnType() const
	{
		return this->Get();
	}
	
	virtual operator Variant() const override
	{
		return Variant(this->Get());
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
	
protected:
	T &operator *()
	{
		if (this->Value == nullptr) {
			throw std::runtime_error("Tried to get the underlying value from a property which has none.");
		}
		
		//get the underlying value
		return *this->Value;
	}

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
	virtual const PropertyBase<T> &operator =(const std::string &rhs) override;
	
	const PropertyBase<T> &operator +=(const T &rhs)
	{
		this->Set(this->Get() + rhs);
		return *this;
	}
	
	const PropertyBase<T> &operator +=(const PropertyBase<T> &rhs)
	{
		return *this += rhs.Get();
	}
	
	virtual const PropertyBase<T> &operator +=(const std::string &rhs) override
	{
		if constexpr(std::is_same_v<T, int>) {
			return *this += std::stoi(rhs);
		} else if constexpr(std::is_same_v<T, bool>) {
			return *this += StringToBool(rhs);
		} else if constexpr(std::is_same_v<T, String>) {
			return *this += String(rhs.c_str());
		} else {
			fprintf(stderr, "The operator += is not valid for this type.");
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
	
	virtual const PropertyBase<T> &operator -=(const std::string &rhs) override
	{
		if constexpr(std::is_same_v<T, int>) {
			return *this -= std::stoi(rhs);
		} else if constexpr(std::is_same_v<T, bool>) {
			return *this -= StringToBool(rhs);
		} else {
			fprintf(stderr, "The operator -= is not valid for this type.");
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

private:
	std::remove_const_t<ReturnType> Get()
	{
		if (this->Getter) {
			if constexpr(std::is_const_v<ReturnType>) {
				return const_cast<std::remove_const_t<ReturnType>>(this->Getter());
			} else {
				return this->Getter();
			}
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
	
private:
	T *Value = nullptr;
	GetterType Getter;
	SetterType Setter;
};

template <typename T, typename O>
class Property : public PropertyBase<T>
{
private:
	Property(const T &value, const GetterType &getter, const SetterType &setter = nullptr) : PropertyBase(value, getter, setter)
	{
	}
	
	Property(const T &value, const SetterType &setter = nullptr) : Property(value, nullptr, setter)
	{
	}
	
	Property(const GetterType &getter, const SetterType &setter = nullptr) : PropertyBase(getter, setter)
	{
	}
	
	Property(const SetterType &setter = nullptr) : Property(T(), nullptr, setter)
	{
	}
	
	friend O;
};

template <typename O>
class Property<String, O> : public PropertyBase<String>
{
private:
	Property(const String &value, const GetterType &getter, const SetterType &setter = nullptr) : PropertyBase(value, getter, setter)
	{
	}
	
	Property(const String &value, const SetterType &setter = nullptr) : Property(value, nullptr, setter)
	{
	}
	
	Property(const GetterType &getter, const SetterType &setter = nullptr) : PropertyBase(getter, setter)
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

template <typename T>
class ExposedPropertyBase : public PropertyCommonBase
{
public:
	using ValueType = T;
	using ReturnType = std::conditional_t<std::is_same_v<T, String>, T &, T>;
	using GetterType = std::function<ReturnType()>;
	using SetterType = std::function<void(const T &)>;
	
protected:
	ExposedPropertyBase(const T &value, const GetterType &getter, const SetterType &setter) : Getter(getter), Setter(setter)
	{
		this->Value = new T;
		*(this->Value) = value;
	}
	
	ExposedPropertyBase(const GetterType &getter, const SetterType &setter) : Getter(getter), Setter(setter)
	{
	}
	
public:
	virtual ~ExposedPropertyBase()
	{
		if (this->Value) {
			delete this->Value;
		}
	}

	const T &operator *() const
	{
		if (this->Value == nullptr) {
			throw std::runtime_error("Tried to get the underlying value from a property which has none.");
		}
		
		//get the underlying value
		return *this->Value;
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
	
	ReturnType operator ->()
	{
		return this->Get();
	}
	
	operator ReturnType() const
	{
		return this->Get();
	}
	
	virtual operator Variant() const override
	{
		return Variant(this->Get());
	}

	bool operator ==(const T &rhs) const
	{
		return this->Get() == rhs;
	}
	
	bool operator ==(const ExposedPropertyBase<T> &rhs) const
	{
		return *this == rhs.Get();
	}
	
	bool operator !=(const T &rhs) const
	{
		return !(*this == rhs);
	}
	
	bool operator !=(const ExposedPropertyBase<T> &rhs) const
	{
		return *this != rhs.Get();
	}
	
	bool operator <(const T &rhs) const
	{
		return this->Get() < rhs;
	}
	
	bool operator <(const ExposedPropertyBase<T> &rhs) const
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
	
	T &operator *()
	{
		if (this->Value == nullptr) {
			throw std::runtime_error("Tried to get the underlying value from a property which has none.");
		}
		
		//get the underlying value
		return *this->Value;
	}

	const ExposedPropertyBase<T> &operator =(const T &rhs)
	{
		this->Set(rhs);
		return *this;
	}
	
	const ExposedPropertyBase<T> &operator =(const ExposedPropertyBase<T> &rhs)
	{
		return *this = rhs.Get();
	}
	
	virtual const ExposedPropertyBase<T> &operator =(const std::string &rhs) override;
	
	const ExposedPropertyBase<T> &operator +=(const T &rhs)
	{
		this->Set(this->Get() + rhs);
		return *this;
	}
	
	const ExposedPropertyBase<T> &operator +=(const ExposedPropertyBase<T> &rhs)
	{
		return *this += rhs.Get();
	}
	
	virtual const ExposedPropertyBase<T> &operator +=(const std::string &rhs) override
	{
		if constexpr(std::is_same_v<T, int>) {
			return *this += std::stoi(rhs);
		} else if constexpr(std::is_same_v<T, bool>) {
			return *this += StringToBool(rhs);
		} else if constexpr(std::is_same_v<T, String>) {
			return *this += String(rhs.c_str());
		} else {
			fprintf(stderr, "The operator += is not valid for this type.");
			return *this;
		}
	}

	const ExposedPropertyBase<T> &operator -=(const T &rhs)
	{
		this->Set(this->Get() - rhs);
		return *this;
	}
	
	const ExposedPropertyBase<T> &operator -=(const ExposedPropertyBase<T> &rhs)
	{
		return *this -= rhs.Get();
	}
	
	virtual const ExposedPropertyBase<T> &operator -=(const std::string &rhs) override
	{
		if constexpr(std::is_same_v<T, int>) {
			return *this -= std::stoi(rhs);
		} else if constexpr(std::is_same_v<T, bool>) {
			return *this -= StringToBool(rhs);
		} else {
			fprintf(stderr, "The operator -= is not valid for this type.");
			return *this;
		}
	}

	const ExposedPropertyBase<T> &operator *=(const T &rhs)
	{
		this->Set(this->Get() * rhs);
		return *this;
	}
	
	const ExposedPropertyBase<T> &operator *=(const ExposedPropertyBase<T> &rhs)
	{
		return *this *= rhs.Get();
	}
	
	virtual const ExposedPropertyBase<T> &operator *=(const std::string &rhs) override
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

	const ExposedPropertyBase<T> &operator /=(const T &rhs)
	{
		this->Set(this->Get() / rhs);
		return *this;
	}
	
	const ExposedPropertyBase<T> &operator /=(const ExposedPropertyBase<T> &rhs)
	{
		return *this /= rhs.Get();
	}
	
	virtual const ExposedPropertyBase<T> &operator /=(const std::string &rhs) override
	{
		if constexpr(std::is_same_v<T, int>) {
			return *this /= std::stoi(rhs);
		} else {
			fprintf(stderr, "The operator /= is not valid for this type.");
			return *this;
		}
	}

protected:
	void Set(const T &value)
	{
		if (this->Setter) {
			this->Setter(value);
		} else if (this->Value != nullptr && *this->Value != value) {
			*this->Value = value;
		}
	}
	
private:
	T *Value = nullptr;
	GetterType Getter;
	SetterType Setter;
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

template <typename T>
inline T operator + (const T &lhs, const PropertyBase<T> &rhs)
{
	return lhs + rhs.Get();
}

template <typename T>
inline T operator + (const T &lhs, const ExposedPropertyBase<T> &rhs)
{
	return lhs + rhs.Get();
}

#endif
