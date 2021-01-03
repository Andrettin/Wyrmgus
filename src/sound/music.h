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
//      (c) Copyright 2020 by Andrettin
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

#include "database/data_entry.h"
#include "database/data_type.h"

#ifdef USE_OAML
class oamlApi;

extern std::unique_ptr<oamlApi> oaml;
extern bool enableOAML;
#endif

namespace wyrmgus {

class sample;
enum class music_type;

class music final : public data_entry, public data_type<music>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::music_type type MEMBER type)
	Q_PROPERTY(int volume_percent MEMBER volume_percent READ get_volume_percent)
	Q_PROPERTY(bool intro MEMBER intro READ is_intro)

public:
	static constexpr const char *class_identifier = "music";
	static constexpr const char *database_folder = "music";

	static const std::vector<const music *> &get_all_of_type(const music_type type)
	{
		static std::vector<const music *> empty_vector;

		const auto find_iterator = music::music_by_type.find(type);
		if (find_iterator != music::music_by_type.end()) {
			return find_iterator->second;
		}

		return empty_vector;
	}

private:
	static inline std::map<music_type, std::vector<const music *>> music_by_type;

public:
	explicit music(const std::string &identifier);
	virtual ~music() override;

	virtual void process_sml_property(const sml_property &property) override;
	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	wyrmgus::sample *get_sample() const
	{
		return this->sample.get();
	}

	int get_volume_percent() const
	{
		return this->volume_percent;
	}

	const std::vector<const music *> &get_submusic() const
	{
		return this->submusic;
	}

	bool is_intro() const
	{
		return this->intro;
	}

private:
	music_type type;
	std::filesystem::path file;
	std::unique_ptr<wyrmgus::sample> sample;
	int volume_percent = 100;
	std::vector<const music *> submusic; //the music pieces grouped under this one
	bool intro = false; //if this is a submusic piece, whether it is an introduction one
};

}
