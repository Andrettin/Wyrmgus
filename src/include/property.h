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

#include <core/ustring.h>
#include <core/variant.h>

#include <functional>
#include <stdexcept>
#include <string>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class DataElement;

template <typename T>
class DataType;

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
	using GetterReturnType = const T&;
	using GetterType = std::function<const T&()>;
	using SetterType = std::function<void(const T&)>;
	
protected:
	PropertyBase(const T &value, const GetterType &getter, const SetterType &setter) : Setter(setter)
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
	
protected:
	const PropertyBase<T> &operator =(const T &rhs)
	{
		this->Set(rhs);
		return *this;
	}
	
	const PropertyBase<T> &operator =(const PropertyBase<T> &rhs)
	{
		return *this = rhs.Get();
	}
	
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

	T &operator *()
	{
		if (this->Value == nullptr) {
			throw std::runtime_error("Tried to get the underlying value from a property which has none.");
		}
		
		//get the underlying value
		return *this->Value;
	}

public:
	const T &operator *() const
	{
		if (this->Value == nullptr) {
			throw std::runtime_error("Tried to get the underlying value from a property which has none.");
		}
		
		//get the underlying value
		return *this->Value;
	}
	
private:
	T &Get()
	{
		if (this->Getter) {
			return const_cast<T &>(this->Getter());
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
	
public:
	const T &Get() const
	{
		//get the underlying value
		if (this->Getter) {
			return this->Getter();
		} else {
			return *this->Value;
		}
	}
	
	operator const T&() const
	{
		return this->Get();
	}
	
	virtual operator Variant() const override
	{
		return Variant(this->Get());
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

#endif
