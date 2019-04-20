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

#include "crtp.h"
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

template <typename T>
class PropertyBase;

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
	
	using AdditionArgumentType = std::conditional_t<is_specialization_of_v<T, std::vector>, const contained_element_t<T> &, const T &>;
	using RemovalArgumentType = std::conditional_t<is_specialization_of_v<T, std::vector>, const contained_element_t<T> &, const T &>;
	using AdderType = std::function<void(AdditionArgumentType)>;
	using RemoverType = std::function<void(RemovalArgumentType)>;
	
protected:
	PropertyBase(const T &value, const GetterType &getter, const SetterType &setter, const AdderType &adder, const RemoverType &remover) : Getter(getter), Setter(setter), Adder(adder), Remover(remover)
	{
		this->Value = new T;
		*(this->Value) = value;
	}
	
	PropertyBase(const GetterType &getter, const SetterType &setter, const AdderType &adder, const RemoverType &remover) : Getter(getter), Setter(setter), Adder(adder), Remover(remover)
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
		if constexpr(is_specialization_of_v<T, std::vector>) {
			return VectorToGodotArray(this->Get());
		} else {
			return Variant(this->Get());
		}
	}

public:	
	ReturnType Get() const
	{
		//get the underlying value
		if (this->Getter) {
			return this->Getter();
		} else {
			return *this->Value;
		}
	}
	
protected:
	T *Value = nullptr;
	GetterType Getter;
	SetterType Setter;
	AdderType Adder;
	RemoverType Remover;
};

template <typename T, typename U>
class ModifiablePropertyBase : public CRTP<U, ModifiablePropertyBase<T, U>>
{
protected:
	ModifiablePropertyBase() {}

public:
	const PropertyBase<T> &operator =(const T &rhs)
	{
		this->Set(rhs);
		return this->GetUnderlying();
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
	const PropertyBase<T> &operator =(const std::string &rhs)
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
			return this->GetUnderlying();
		}
	}
	
	const PropertyBase<T> &operator +=(const T &rhs)
	{
		this->Add(rhs);
		return this->GetUnderlying();
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
	const PropertyBase<T> &operator +=(const std::string &rhs)
	{
		if constexpr(
			std::is_same_v<T, int>
			|| std::is_same_v<T, bool>
			|| std::is_same_v<T, String>
		) {
			return *this += ConvertFromString<T>(rhs);
		} else if constexpr(is_specialization_of_v<T, std::vector>) {
			typename T::value_type new_value = ConvertFromString<typename T::value_type>(rhs);
			this->Add(new_value);
			return this->GetUnderlying();
		} else {
			fprintf(stderr, "The operator += is not valid for this type.\n");
			return this->GetUnderlying();
		}
	}

	const PropertyBase<T> &operator -=(const T &rhs)
	{
		this->Remove(rhs);
		return this->GetUnderlying();
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
	const PropertyBase<T> &operator -=(const std::string &rhs)
	{
		if constexpr(
			std::is_same_v<T, int>
			|| std::is_same_v<T, bool>
		) {
			return *this -= ConvertFromString<T>(rhs);
		} else if constexpr(is_specialization_of_v<T, std::vector>) {
			typename T::value_type new_value = ConvertFromString<typename T::value_type>(rhs);
			this->Remove(new_value);
			return this->GetUnderlying();
		} else {
			fprintf(stderr, "The operator -= is not valid for this type.\n");
			return this->GetUnderlying();
		}
	}
	
	const PropertyBase<T> &operator *=(const T &rhs)
	{
		this->Set(this->GetUnderlying().Get() * rhs);
		return this->GetUnderlying();
	}
	
	const PropertyBase<T> &operator *=(const PropertyBase<T> &rhs)
	{
		return *this *= rhs.Get();
	}

	const PropertyBase<T> &operator /=(const T &rhs)
	{
		this->Set(this->GetUnderlying().Get() / rhs);
		return this->GetUnderlying();
	}
	
	const PropertyBase<T> &operator /=(const PropertyBase<T> &rhs)
	{
		return *this /= rhs.Get();
	}
	
	void Set(const T &value)
	{
		if (this->GetUnderlying().Setter) {
			this->GetUnderlying().Setter(value);
		} else if (this->GetUnderlying().Value != nullptr && *this->GetUnderlying().Value != value) {
			*this->GetUnderlying().Value = value;
		}
	}
	
	void Add(typename PropertyBase<T>::AdditionArgumentType value)
	{
		if (this->GetUnderlying().Adder) {
			this->GetUnderlying().Adder(value);
		} else {
			if constexpr(is_specialization_of_v<T, std::vector>) {
				this->GetModifiable().push_back(value);
			} else {
				this->Set(this->GetUnderlying().Get() + value);
			}
		}
	}
	
	void Remove(typename PropertyBase<T>::RemovalArgumentType value)
	{
		if (this->GetUnderlying().Remover) {
			this->GetUnderlying().Remover(value);
		} else {
			if constexpr(is_specialization_of_v<T, std::vector>) {
				this->GetModifiable().erase(std::remove(this->GetModifiable().begin(), this->GetModifiable().end(), value), this->GetModifiable().end());
			} else {
				this->Set(this->GetUnderlying().Get() - value);
			}
		}
	}
	
	typename PropertyBase<T>::ModifiableReturnType GetModifiable() const
	{
		if constexpr(std::is_reference_v<typename PropertyBase<T>::ReturnType>) {
			return const_cast<typename PropertyBase<T>::ModifiableReturnType>(this->GetUnderlying().Get());
		} else {
			return this->GetUnderlying().Get();
		}
	}
};

template <typename T, typename U>
class ReadablePropertyBase : public CRTP<U, ReadablePropertyBase<T, U>>
{
protected:
	ReadablePropertyBase() {}

public:
	bool operator ==(const T &rhs) const
	{
		return this->GetUnderlying().Get() == rhs;
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
		return this->GetUnderlying().Get() < rhs;
	}
	
	bool operator <(const PropertyBase<T> &rhs) const
	{
		return *this < rhs.Get();
	}
	
	T operator +(const T &rhs)
	{
		return this->GetUnderlying().Get() + rhs;
	}
	
	T operator +(const PropertyBase<T> &rhs)
	{
		return *this + rhs.Get();
	}
	
	T operator -(const T &rhs)
	{
		return this->GetUnderlying().Get() - rhs;
	}
	
	T operator -(const PropertyBase<T> &rhs)
	{
		return *this - rhs.Get();
	}
	
	T operator *(const T &rhs)
	{
		return this->GetUnderlying().Get() * rhs;
	}
	
	T operator *(const PropertyBase<T> &rhs)
	{
		return *this * rhs.Get();
	}
	
	T operator /(const T &rhs)
	{
		return this->GetUnderlying().Get() / rhs;
	}
	
	T operator /(const PropertyBase<T> &rhs)
	{
		return *this / rhs.Get();
	}
};

template <typename T>
class ProtectedPropertyBase : public PropertyBase<T>, protected ModifiablePropertyBase<T, ProtectedPropertyBase<T>>, public ReadablePropertyBase<T, ProtectedPropertyBase<T>>
{
protected:
	ProtectedPropertyBase(const T &value, const GetterType &getter, const SetterType &setter, const AdderType &adder, const RemoverType &remover) : PropertyBase(value, getter, setter, adder, remover)
	{
	}
	
	ProtectedPropertyBase(const GetterType &getter, const SetterType &setter, const AdderType &adder, const RemoverType &remover) : PropertyBase(getter, setter, adder, remover)
	{
	}

protected:
	ModifiablePointerType operator ->()
	{
		if constexpr(!is_fundamental_v<T>) {
			return &this->Get();
		} else {
			return nullptr;
		}
	}
	
public:
	PointerType operator ->() const
	{
		if constexpr(!is_fundamental_v<T>) {
			return &this->Get();
		} else {
			return nullptr;
		}
	}
	
	operator ReturnType() const
	{
		return this->Get();
	}
	
private:
	virtual const PropertyBase<T> &operator =(const std::string &rhs) override
	{
		return ModifiablePropertyBase<T, ProtectedPropertyBase<T>>::operator =(rhs);
	}
	
	virtual const PropertyBase<T> &operator +=(const std::string &rhs) override
	{
		return ModifiablePropertyBase<T, ProtectedPropertyBase<T>>::operator +=(rhs);
	}

	virtual const PropertyBase<T> &operator -=(const std::string &rhs) override
	{
		return ModifiablePropertyBase<T, ProtectedPropertyBase<T>>::operator -=(rhs);
	}
	
	friend ModifiablePropertyBase<T, ProtectedPropertyBase<T>>;
	friend ReadablePropertyBase<T, ProtectedPropertyBase<T>>;
};

template <typename T>
class ProtectedPropertyBase<T *> : public PropertyBase<T *>, protected ModifiablePropertyBase<T *, ProtectedPropertyBase<T *>>, public ReadablePropertyBase<T *, ProtectedPropertyBase<T *>>
{
public:
	using ValueType = typename PropertyBase<T *>::ValueType;
	using PointerType = typename PropertyBase<T *>::PointerType;
	using ReturnType = typename PropertyBase<T *>::ReturnType;
	using ArgumentType = typename PropertyBase<T *>::ArgumentType;
	using GetterType = typename PropertyBase<T *>::GetterType;
	using SetterType = typename PropertyBase<T *>::SetterType;
	
	using AdderType = typename PropertyBase<T *>::AdderType;
	using RemoverType = typename PropertyBase<T *>::RemoverType;
	
protected:
	ProtectedPropertyBase(const ValueType &value, const GetterType &getter, const SetterType &setter, const AdderType &adder, const RemoverType &remover) : PropertyBase(value, getter, setter, adder, remover)
	{
	}
	
	ProtectedPropertyBase(const GetterType &getter, const SetterType &setter, const AdderType &adder, const RemoverType &remover) : PropertyBase(getter, setter, adder, remover)
	{
	}

public:
	PointerType operator ->() const
	{
		return this->Get();
	}
	
	operator ReturnType() const
	{
		return this->Get();
	}
	
private:
	virtual const PropertyBase<ValueType> &operator =(const std::string &rhs) override
	{
		return ModifiablePropertyBase<ValueType, ProtectedPropertyBase<ValueType>>::operator =(rhs);
	}
	
	virtual const PropertyBase<ValueType> &operator +=(const std::string &rhs) override
	{
		return ModifiablePropertyBase<ValueType, ProtectedPropertyBase<ValueType>>::operator +=(rhs);
	}

	virtual const PropertyBase<ValueType> &operator -=(const std::string &rhs) override
	{
		return ModifiablePropertyBase<ValueType, ProtectedPropertyBase<ValueType>>::operator -=(rhs);
	}
	
	friend ModifiablePropertyBase<ValueType, ProtectedPropertyBase<ValueType>>;
	friend ReadablePropertyBase<ValueType, ProtectedPropertyBase<ValueType>>;
};

template <typename T, typename O>
class Property : public ProtectedPropertyBase<T>
{
private:
	Property(const T &value, const GetterType &getter, const SetterType &setter = nullptr) : ProtectedPropertyBase(value, getter, setter, nullptr, nullptr)
	{
	}
	
	Property(const T &value, const SetterType &setter = nullptr) : Property(value, nullptr, setter)
	{
	}
	
	Property(const GetterType &getter, const SetterType &setter = nullptr) : ProtectedPropertyBase(getter, setter, nullptr, nullptr)
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
	Property(const String &value, const GetterType &getter, const SetterType &setter = nullptr) : ProtectedPropertyBase(value, getter, setter, nullptr, nullptr)
	{
	}
	
	Property(const String &value, const SetterType &setter = nullptr) : Property(value, nullptr, setter)
	{
	}
	
	Property(const GetterType &getter, const SetterType &setter = nullptr) : ProtectedPropertyBase(getter, setter, nullptr, nullptr)
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
	using ValueType = typename PropertyBase<std::vector<T>>::ValueType;
	using ReturnType = typename PropertyBase<std::vector<T>>::ReturnType;
	using ArgumentType = typename PropertyBase<std::vector<T>>::ArgumentType;
	using GetterType = typename PropertyBase<std::vector<T>>::GetterType;
	using SetterType = typename PropertyBase<std::vector<T>>::SetterType;
	
	using AdditionArgumentType = typename PropertyBase<std::vector<T>>::AdditionArgumentType;
	using RemovalArgumentType = typename PropertyBase<std::vector<T>>::RemovalArgumentType;
	using AdderType = typename PropertyBase<std::vector<T>>::AdderType;
	using RemoverType = typename PropertyBase<std::vector<T>>::RemoverType;
	
private:
	Property(const std::vector<T> &value, const GetterType &getter, const AdderType &adder = nullptr, const RemoverType &remover = nullptr) : ProtectedPropertyBase(value, getter, nullptr, adder, remover)
	{
	}
	
	Property(const std::vector<T> &value, const AdderType &adder = nullptr, const RemoverType &remover = nullptr) : Property(value, nullptr, adder, remover)
	{
	}
	
	Property(const GetterType &getter, const AdderType &adder = nullptr, const RemoverType &remover = nullptr) : ProtectedPropertyBase(getter, nullptr, adder, remover)
	{
	}
	
	Property(const AdderType &adder = nullptr, const RemoverType &remover = nullptr) : Property(std::vector<T>(), nullptr, adder, remover)
	{
	}
	
public:
	typename std::vector<T>::const_reference operator [](typename std::vector<T>::size_type pos) const
	{
		return this->Get()[pos];
	}
	
	bool empty() const
	{
		return this->Get().empty();
	}
	
	typename std::vector<T>::size_type size() const
	{
		return this->Get().size();
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
	typename std::vector<T>::reference operator [](typename std::vector<T>::size_type pos)
	{
		return this->GetModifiable()[pos];
	}
	
	void push_back(AdditionArgumentType value)
	{
		return this->Add(value);
	}
	
	void clear()
	{
		return this->GetModifiable().clear();
	}
	
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
class ExposedPropertyBase : public PropertyBase<T>, public ModifiablePropertyBase<T, ExposedPropertyBase<T>>, public ReadablePropertyBase<T, ExposedPropertyBase<T>>
{
protected:
	ExposedPropertyBase(const T &value, const GetterType &getter, const SetterType &setter, const AdderType &adder, const RemoverType &remover) : PropertyBase(value, getter, setter, adder, remover)
	{
	}
	
	ExposedPropertyBase(const GetterType &getter, const SetterType &setter, const AdderType &adder, const RemoverType &remover) : PropertyBase(getter, setter, adder, remover)
	{
	}

public:
	ModifiablePointerType operator ->()
	{
		if constexpr(std::is_pointer_v<T>) {
			return this->GetModifiable();
		} else if constexpr(!is_fundamental_v<T>) {
			return &this->GetModifiable();
		} else {
			return nullptr;
		}
	}
	
	operator ModifiableReturnType()
	{
		return this->GetModifiable();
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
	
private:
	virtual const PropertyBase<T> &operator =(const std::string &rhs) override
	{
		return ModifiablePropertyBase<T, ExposedPropertyBase<T>>::operator =(rhs);
	}
	
	virtual const PropertyBase<T> &operator +=(const std::string &rhs) override
	{
		return ModifiablePropertyBase<T, ExposedPropertyBase<T>>::operator +=(rhs);
	}

	virtual const PropertyBase<T> &operator -=(const std::string &rhs) override
	{
		return ModifiablePropertyBase<T, ExposedPropertyBase<T>>::operator -=(rhs);
	}
	
	friend ModifiablePropertyBase<T, ExposedPropertyBase<T>>;
	friend ReadablePropertyBase<T, ExposedPropertyBase<T>>;
};

/**
**	@brief	A property which can be written to publicly
*/
template <typename T, typename O>
class ExposedProperty : public ExposedPropertyBase<T>
{
private:
	ExposedProperty(const T &value, const GetterType &getter, const SetterType &setter = nullptr) : ExposedPropertyBase(value, getter, setter, nullptr, nullptr)
	{
	}
	
	ExposedProperty(const T &value, const SetterType &setter = nullptr) : ExposedProperty(value, nullptr, setter)
	{
	}
	
	ExposedProperty(const GetterType &getter, const SetterType &setter = nullptr) : ExposedPropertyBase(getter, setter, nullptr, nullptr)
	{
	}
	
	ExposedProperty(const SetterType &setter = nullptr) : ExposedProperty(T(), nullptr, setter)
	{
	}
	
public:
	const PropertyBase<T> &operator =(const T &rhs)
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
	ExposedProperty(const String &value, const GetterType &getter, const SetterType &setter = nullptr) : ExposedPropertyBase(value, getter, setter, nullptr, nullptr)
	{
	}
	
	ExposedProperty(const String &value, const SetterType &setter = nullptr) : ExposedProperty(value, nullptr, setter)
	{
	}
	
	ExposedProperty(const GetterType &getter, const SetterType &setter = nullptr) : ExposedPropertyBase(getter, setter, nullptr, nullptr)
	{
	}
	
	ExposedProperty(const SetterType &setter = nullptr) : ExposedProperty(String(), nullptr, setter)
	{
	}
	
public:
	const PropertyBase<String> &operator =(const String &rhs)
	{
		this->Set(rhs);
		return *this;
	}
	
	const PropertyBase<String> &operator =(const char *rhs)
	{
		return *this = String(rhs);
	}
	
	const PropertyBase<String> &operator =(const std::string &rhs)
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
	using ValueType = typename PropertyBase<std::vector<T>>::ValueType;
	using ReturnType = typename PropertyBase<std::vector<T>>::ReturnType;
	using ArgumentType = typename PropertyBase<std::vector<T>>::ArgumentType;
	using GetterType = typename PropertyBase<std::vector<T>>::GetterType;
	using SetterType = typename PropertyBase<std::vector<T>>::SetterType;
	
	using AdditionArgumentType = typename PropertyBase<std::vector<T>>::AdditionArgumentType;
	using RemovalArgumentType = typename PropertyBase<std::vector<T>>::RemovalArgumentType;
	using AdderType = typename PropertyBase<std::vector<T>>::AdderType;
	using RemoverType = typename PropertyBase<std::vector<T>>::RemoverType;
	
private:
	ExposedProperty(const std::vector<T> &value, const GetterType &getter, const AdderType &adder = nullptr, const RemoverType &remover = nullptr) : ExposedPropertyBase(value, getter, nullptr, adder, remover)
	{
	}
	
	ExposedProperty(const std::vector<T> &value, const AdderType &adder = nullptr, const RemoverType &remover = nullptr) : ExposedProperty(value, nullptr, adder, remover)
	{
	}
	
	ExposedProperty(const GetterType &getter, const AdderType &adder = nullptr, const RemoverType &remover = nullptr) : ExposedPropertyBase(getter, nullptr, adder, remover)
	{
	}
	
	ExposedProperty(const AdderType &adder = nullptr, const RemoverType &remover = nullptr) : ExposedProperty(std::vector<T>(), nullptr, adder, remover)
	{
	}
	
public:
	typename std::vector<T>::reference operator [](typename std::vector<T>::size_type pos)
	{
		return this->GetModifiable()[pos];
	}
	
	typename std::vector<T>::const_reference operator [](typename std::vector<T>::size_type pos) const
	{
		return this->Get()[pos];
	}
	
	void push_back(AdditionArgumentType value)
	{
		return this->Add(value);
	}
	
	void clear()
	{
		return this->GetModifiable().clear();
	}
	
	bool empty() const
	{
		return this->Get().empty();
	}
	
	typename std::vector<T>::size_type size() const
	{
		return this->Get().size();
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
		return this->GetModifiable().begin();
	}
	
	typename std::vector<T>::iterator end()
	{
		return this->GetModifiable().end();
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
